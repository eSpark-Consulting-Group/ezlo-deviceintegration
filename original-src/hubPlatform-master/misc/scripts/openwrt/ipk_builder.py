#!/usr/bin/env python3

import os
import shutil
import sys
import subprocess
import hashlib


class Constraint:
    def __init__(self, operator='=', version='current'):
        self.operator = operator
        self.version = version

    def __eq__(self, other):
        return self.operator == other.operator and self.version == other.version

    def __ne__(self, other):
        return not self.__eq__(other)


class Package:
    def __init__(self, name, version, architecture='all'):
        self.__name = name
        self.__version = version
        self.__architecture = architecture
        self.__regenerate_file_name()
        self.dependencies = set()
        self.dependency_constraints = dict()
        self.section = 'eZLO Innovation'
        self.installed_size = 0
        self.description = 'Just ' + self.name + ' package'
        self.size = 0
        self.sha256sum = ''

    @property
    def version(self):
        return self.__version

    @version.setter
    def version(self, value):
        self.__version = value
        self.__regenerate_file_name()

    @property
    def name(self):
        return self.__name

    @name.setter
    def name(self, value):
        self.__name = value
        self.__regenerate_file_name()

    @property
    def architecture(self):
        return self.__architecture

    @architecture.setter
    def architecture(self, value):
        self.__architecture = value
        self.__regenerate_file_name()

    def __regenerate_file_name(self):
        self.file_name = self.name + '_' + self.version + '_' + self.architecture + '.ipk'


class IpkBuilder:
    def __init__(self, tmp_dir, pkgs_dir, should_generate_index):
        self.tmp_dir = tmp_dir
        self.pkgs_dir = pkgs_dir
        self.should_generate_index = should_generate_index
        if self.should_generate_index:
            self.index_file_path = os.path.join(pkgs_dir, 'Packages')
            self.utils_dir = os.path.dirname(os.path.realpath(__file__))

        self.__check_system_utils()

        if os.path.isdir(self.tmp_dir):
            shutil.rmtree(self.tmp_dir)
        os.makedirs(self.tmp_dir)

        if os.path.isdir(self.pkgs_dir):
            shutil.rmtree(self.pkgs_dir)
        os.makedirs(self.pkgs_dir)

    def __del__(self):
        if self.should_generate_index:
            self.__workaround_usign_bug(self.index_file_path)
            # Create gzipped index file
            index_file_gz = open(self.index_file_path + '.gz', "w")
            subprocess.call(["gzip", "-nc9",  self.index_file_path], stdout=index_file_gz)
            # Sign index file
            subprocess.run([os.path.join(self.utils_dir, 'usign'), '-S', '-m', self.index_file_path,
                            '-s', os.path.join(self.utils_dir, 'key-build')])

    @staticmethod
    def __workaround_usign_bug(index_file):
        # Details: https://github.com/openwrt/openwrt/commit/e1f588e446c7ceb696b644b37aeab9b3476e2a57
        if 64 + os.path.getsize(index_file) % 128 in [110, 111]:
            print('WARNING: Applying padding in index file to workaround usign SHA-512 bug!')
            with open(index_file, 'a') as index:
                index.write('\n\n')

    @staticmethod
    def __sha256sum(filepath):
        h = hashlib.sha256()
        b = bytearray(128*1024)
        mv = memoryview(b)
        with open(filepath, 'rb', buffering=0) as f:
            for n in iter(lambda: f.readinto(mv), 0):
                h.update(mv[:n])
        return h.hexdigest()

    @staticmethod
    def __generate_control_file(package, index=False):
        dependencies = ''
        for dependency in sorted(package.dependencies):
            for constraint in package.dependency_constraints[dependency]:
                if dependencies:
                    dependencies += ', '
                dependencies += '{0} ({1}{2})'.format(dependency, constraint.operator, constraint.version)

        if dependencies:
            dependencies += ', '
        dependencies += 'libc'

        for dependency in sorted(package.dependencies):
            if dependencies:
                dependencies += ', '
            dependencies += dependency

        index_part = ''
        if index:
            index_part = ('Filename: {0}\n'
                          'Size: {1}\n'
                          'SHA256sum: {2}\n').format(package.file_name, package.size, package.sha256sum)

        control_file_content = ('Package: {0}\n'
                                'Version: {1}\n'
                                'Depends:  {2}\n'
                                'Section: {3}\n'
                                'Architecture: {4}\n'
                                'Installed-Size: {5}\n'
                                '{6}Description:  {7}\n').format(package.name,
                                                                 package.version,
                                                                 dependencies,
                                                                 package.section,
                                                                 package.architecture,
                                                                 package.installed_size,
                                                                 index_part,
                                                                 package.description)

        if index:
            control_file_content += '\n'

        return control_file_content

    @staticmethod
    def __check_command_exist(command):
        print('Checking {}: '.format(command), end='', flush=True)
        if subprocess.run(['bash', '-c', 'command -v {}'.format(command)]).returncode != 0:
            print('not found')
            return False
        return True

    def __check_system_utils(self):
        print('Checking if system utils exist:')
        required_commands = ['tar', 'gzip'] if self.should_generate_index else ['tar']
        if not all(map(lambda cmd: self.__check_command_exist(cmd), required_commands)):
            print('Exiting...')
            exit(1)

    def __get_pkg_dir(self, package):
        return os.path.join(self.tmp_dir, package.name)

    def get_pkg_data_dir(self, package):
        return os.path.join(self.__get_pkg_dir(package), 'data')

    def create_ipk(self, package):
        # Create plugin package data structure
        pkg_dir = self.__get_pkg_dir(package)
        pkg_data_dir = self.get_pkg_data_dir(package)

        # Calculate package installed size
        package.installed_size = sum([sum(map(lambda fname: os.lstat(os.path.join(directory, fname)).st_size, files))
                                      for directory, folders, files in os.walk(pkg_data_dir)])

        # Create plugin package control file
        pkg_control_dir = os.path.join(pkg_dir, 'control')
        os.makedirs(pkg_control_dir)
        open(os.path.join(pkg_control_dir, 'control'), 'w').write(self.__generate_control_file(package))

        open(os.path.join(pkg_control_dir, 'prerm'), 'w').write('#!/bin/sh\n'
                                                                '[ -x ${IPKG_INSTROOT}/lib/functions.sh ] || exit 0\n'
                                                                '. ${IPKG_INSTROOT}/lib/functions.sh\n'
                                                                'default_prerm $0 $@\n')
        os.chmod(os.path.join(pkg_control_dir, 'prerm'), 0o755)

        open(os.path.join(pkg_control_dir, 'postinst'), 'w').write('#!/bin/sh\n'
                                                                   '[ "${IPKG_NO_SCRIPT}" = "1" ] && exit 0\n'
                                                                   '[ -x ${IPKG_INSTROOT}/lib/functions.sh ] || exit 0\n'
                                                                   '. ${IPKG_INSTROOT}/lib/functions.sh\n'
                                                                   'default_postinst $0 $@\n')
        os.chmod(os.path.join(pkg_control_dir, 'postinst'), 0o755)

        # Create plugin package debian-binary file
        open(os.path.join(pkg_dir, 'debian-binary'), 'w').write('2.0\n')

        # Create .ipk file
        original_dir = os.getcwd()
        if not os.path.isdir(pkg_data_dir):
            os.makedirs(pkg_data_dir)
        os.chdir(pkg_data_dir)
        subprocess.run(['tar', '--owner=0', '--group=0', '--format=gnu', '--sort=name', '-czf', '../data.tar.gz', '.'])
        os.chdir(pkg_control_dir)
        subprocess.run(['tar', '--owner=0', '--group=0', '--format=gnu', '--sort=name', '-czf', '../control.tar.gz', '.'])
        os.chdir(pkg_dir)
        pkg_file_path = os.path.join(self.pkgs_dir, package.file_name)
        subprocess.run(['tar', '--owner=0', '--group=0', '--format=gnu', '--sort=name', '-czf',
                        pkg_file_path, 'debian-binary', 'data.tar.gz', 'control.tar.gz'])

        os.chdir(original_dir)

        # Append package to index
        if self.should_generate_index:
            package.size = os.path.getsize(pkg_file_path)
            package.sha256sum = self.__sha256sum(pkg_file_path)
            index_package_content = self.__generate_control_file(package, True)
            open(self.index_file_path, 'a').write(index_package_content)


def main():
    return 0


if __name__ == '__main__':
    sys.exit(main())
