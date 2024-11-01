#!/usr/bin/env python3

import os
import sys
import subprocess
import re
import argparse
import enum
import shutil
import difflib
import ipk_builder


class Constraint:
    def __init__(self, operator='=', version='current'):
        self.operator = operator
        self.version = version

    def __eq__(self, other):
        return self.operator == other.operator and self.version == other.version

    def __ne__(self, other):
        return not self.__eq__(other)


class Package(ipk_builder.Package):
    default_version = '1.0'

    def __init__(self, name, installed=False, meta=False, has_makefile=False):
        ipk_builder.Package.__init__(self, name, self.default_version, )
        self.installed = installed
        self.meta = meta
        self.has_makefile = has_makefile
        self.description = '\t' + self.description + '\n'
        self.config_section = ''
        self.install_section = ''
        self.conffiles = set()


system_library_packages = {
    'libatomic.so.1':                   'libatomic',
    'libBasicUsageEnvironment.so.1':    'libliveMedia',
    'libgroupsock.so.8':                'libliveMedia',
    'libliveMedia.so.64':               'libliveMedia',
    'libUsageEnvironment.so.3':         'libliveMedia',
    'libc.so':                          'libc',
    'libcrypto.so.1.0.0':               'libopenssl',
    'libssl.so.1.0.0':                  'libopenssl',
    'libcurl.so.4':                     'libcurl',
    'libdbus-1.so.3':                   'libdbus',
    'libgcc_s.so.1':                    'libgcc',
    'libglib-2.0.so.0':                 'glib2',
    'libixml.so.2':                     'libupnp',
    'libthreadutil.so.6':               'libupnp',
    'libupnp.so.6':                     'libupnp',
    'liblmdb.so':                       'openldap-LMDB',
    'liblua.so.5.3':                    'liblua',
    'libminilzo2.so':                   'libminilzo',
    'libmongoose.so':                   'libmongoose',
    'libnetlink.so':                    'ip',
    'libstdc++.so.6':                   'libstdcpp',
    'libtspread-core.so.3':             'libspread',
    'libuv.so.1':                       'libuv',
    'libwebsockets.so.14':              'libwebsockets-custom',
    'libz.so.1':                        'zlib',
    'ld-uClibc.so.0':                   'libc',
    'libc.so.0':                        'libc',
    'libdl.so.0':                       'libc',
    'libm.so.0':                        'libc',
    'libpthread.so.0':                  'libpthread',
    'librt.so.0':                       'librt',
    'libffi.so.6':                      'libffi'
}

system_package_constraints = {
    'common': {
        'glib2':                            [Constraint('=', '2.46.2-1')],
        'ip':                               [Constraint('=', '4.4.0-1')],
        'libcurl':                          [Constraint('=', '7.58.0-1')],
        'libdbus':                          [Constraint('=', '1.9.14-1')],
        'libliveMedia':                     [Constraint('=', '2018.10.17-1')],
        'liblua':                           [Constraint('=', '5.3.5-1')],
        'libminilzo':                       [Constraint('=', '2.10-1')],
        'libmongoose':                      [Constraint('=', '2.8-1')],
        'libopenssl':                       [Constraint('=', '1.0.2p-1')],
        'libspread':                        [Constraint('=', '4.4.0-1')],
        'libupnp':                          [Constraint('=', '1.6.19-1')],
        'libuv':                            [Constraint('=', '1.20.2_modified-1')],
        'libwebsockets-custom':             [Constraint('=', '3.1.0-1')],
        'openldap-LMDB':                    [Constraint('=', '0.9.22-1')],
        'zlib':                             [Constraint('=', '1.2.8-1')]
    },
    'arm_cortex-a7_neon-vfpv4': {
        'libc':                             [Constraint('=', '1.1.23-2')],
        'libgcc':                           [Constraint('=', '7.4.0-2')],
        'libstdcpp':                        [Constraint('=', '7.4.0-2')],
        'libatomic':                        [Constraint('=', '7.4.0-2')]
    },
    'ramips_24kec': {
        'libc':                             [Constraint('=', '0.9.33.2-1')],
        'libgcc':                           [Constraint('=', '4.8-linaro-1')],
        'libstdcpp':                        [Constraint('=', '4.8-linaro-1')],
        'libatomic':                        [Constraint('=', '4.8-linaro-1')],
        'libpthread':                       [Constraint('=', '0.9.33.2-1')],
        'librt':                            [Constraint('=', '0.9.33.2-1')],
        'libffi':                           [Constraint('=', '3.0.13-1')]
    },
    'mipsel_24kc': {
        'libc':                             [Constraint('=', '1.1.23-2')],
        'libgcc':                           [Constraint('=', '7.4.0-2')],
        'libstdcpp':                        [Constraint('=', '7.4.0-2')],
        'libatomic':                        [Constraint('=', '7.4.0-2')]
    }
}


conffiles = [
    '/opt/firmware/etc/house_modes.cfg'
]

TARGET_NAME_H2_1        = 'arm-musl-7.4.0_minsizerel-h2.1'
TARGET_NAME_H2_SECURE_1 = 'arm-musl-7.4.0_minsizerel-h2_secure.1'
TARGET_NAME_G150        = 'mips-uclibc-4.8-linaro_minsizerel-g150'
TARGET_NAME_BEEWAVE_1   = 'mips-musl-7.4.0_minsizerel-beewave.1'

# toolchain_name (toolchain name + configuration) -- ${CONFIG_ARCH}-${CONFIG_LIBC}-${CONFIG_GCC_VERSION}
# right now, hubPlatform's openwrt package mks depend only on an openwrt's toolchain
target_names = {
    TARGET_NAME_H2_1:        ( 'arm', 'musl', '7.4.0', "h2.1" ),
    TARGET_NAME_H2_SECURE_1: ( 'arm', 'musl', '7.4.0', "h2_secure.1" ),
    TARGET_NAME_G150:        ( 'mipsel', 'uClibc', '4.8-linaro', "g150" ),
    TARGET_NAME_BEEWAVE_1:   ( 'mipsel', 'musl', '7.4.0', "beewave.1" )
}

architecture_names = {
    TARGET_NAME_H2_1:        'arm_cortex-a7_neon-vfpv4',
    TARGET_NAME_H2_SECURE_1: 'arm_cortex-a7_neon-vfpv4',
    TARGET_NAME_G150:        'ramips_24kec',
    TARGET_NAME_BEEWAVE_1:   'mipsel_24kc'
}

hub_types_map = {
    'h2.1':        TARGET_NAME_H2_1,
    'h2_secure.1': TARGET_NAME_H2_SECURE_1,
    'g150':        TARGET_NAME_G150,
    'beewave.1':   TARGET_NAME_BEEWAVE_1
}

# TODO remove this temporary fix after webrtc will be integrated with system libraries
# packages that have their own self contained set of libraries
# that are not depend on system libraties
self_contained_packages = { "firmware-common-webrtcd" }

def is_target_name(target_name):
    if target_name not in target_names:
        return False
    else:
        return True


def is_system_package(package_name):
    for name, deps_dict in system_package_constraints.items():
        if package_name in deps_dict:
            return True
    return False


def get_system_package_constraint(target_name, package_name):
    if not is_system_package(package_name):
        print("Requested package isn't a system one", file=sys.stderr)
        sys.exit(1)
    if not is_target_name(target_name):
        print('Unsupported target name, use the next ones: ', list(sorted(target_names.keys())), file=sys.stderr)
        sys.exit(1)    

    # look-up at first as a target specific dependency
    constraint = system_package_constraints[architecture_names[target_name]].get(package_name)
    if constraint is not None:
        return constraint

    return system_package_constraints['common'][package_name]


def is_hub_platform_package(package_name):
    return package_name.startswith('firmware-') or \
           package_name.startswith('hub-platform-')


# packages that are baked within firmware image
def is_hub_platform_firmware_package(package_name):
    return package_name.startswith('firmware-common-') or \
           package_name.startswith('firmware-core-') or \
           package_name.startswith('firmware-utils-')


def fill_library_packages(predefined_library_packages, input_directory):
    library_packages = predefined_library_packages
    for package_name in os.listdir(input_directory):
        if package_name.count('-') == 2:
            if package_name in self_contained_packages:
                continue
            for _, _, library_names in os.walk(os.path.join(input_directory, package_name)):
                for library_name in library_names:
                    if '.so' in library_name:
                        library_packages[library_name] = package_name
    return library_packages


def fill_dependencies_and_conffiles(library_packages, input_directory):
    unresolved_dependencies = set()
    packages = dict()

    for package_name in os.listdir(input_directory):
        if package_name.count('-') == 2:
            package = Package(package_name, installed=True)
            packages[package_name] = package
            if package_name in self_contained_packages:
                continue
            for path_to_binary, _, binaries in os.walk(os.path.join(input_directory, package_name)):
                for binary in binaries:
                    # read dependencies for executables and libraries
                    result = subprocess.run(['readelf', '-d', os.path.join(path_to_binary, binary)],
                                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
                    if result.returncode == 0:
                        for line in result.stdout.decode('utf-8').splitlines():
                            m = re.search('.*NEEDED.*\[(.+)\].*', line)
                            if m:
                                dependent_library_name = m.group(1)
                                if dependent_library_name in library_packages:
                                    if library_packages[dependent_library_name] != package.name:
                                        package.dependencies.add(library_packages[dependent_library_name])
                                else:
                                    unresolved_dependencies.add(dependent_library_name)
                    else:
                        conffile = '/' + os.path.relpath(os.path.join(path_to_binary, binary),
                                                         os.path.join(input_directory, package_name))
                        if conffile in conffiles:
                            package.conffiles.add(conffile)

    return unresolved_dependencies, packages


def have_cycles(packages):
    result = False
    visited = set()
    path = []
    path_set = set()
    stack = [iter(packages.keys())]
    while stack:
        for v in stack[-1]:
            if v in path_set:
                result = True
                print('Cycle detected:', path[path.index(v):], file=sys.stderr)
            elif v not in visited:
                visited.add(v)
                path.append(v)
                path_set.add(v)
                stack.append(iter(packages.get(v, Package('dummy')).dependencies))
                break
        else:
            if path:
                path_set.remove(path.pop())
            stack.pop()

    return result


def ensure_meta_package(packages, package_name):
    if package_name not in packages.keys():
        packages[package_name] = Package(package_name, meta=True)
    else:
        packages[package_name].meta = True

    return packages[package_name]


def update_meta_packages(target_name, packages):
    # Collect common and core packages into hub-platform-firmware.
    # If anyone depends on those packages, change dependency to hub-platform-firmware
    firmware_meta_package = ensure_meta_package(packages, 'hub-platform-firmware')

    for package_name in sorted(packages.keys()):
        if is_hub_platform_firmware_package(package_name) and packages[package_name].installed:
            firmware_meta_package.dependencies.add(package_name)

    for package_name in sorted(packages.keys()):
        if package_name == firmware_meta_package.name:
            continue

        if package_name not in firmware_meta_package.dependencies:
            packages[package_name].dependencies = set(map(lambda x: firmware_meta_package.name if x in firmware_meta_package.dependencies else x, packages[package_name].dependencies))


def parse_existing_makefiles(packages, opkg_target_dir):
    for makefile_name in os.listdir(opkg_target_dir):
        if not makefile_name.endswith('.mk'):
            if os.path.isfile(os.path.join(opkg_target_dir, makefile_name)):
                print('Skipping', os.path.join(opkg_target_dir, makefile_name), 'file...', file=sys.stderr)
            continue

        package_name = makefile_name[0:-3]
        if package_name in packages.keys():
            package = packages[package_name]
            package.has_makefile = True
        else:
            package = Package(package_name, has_makefile=True)
            packages[package_name] = package

        State = enum.Enum('State', 'WHATEVER DEFINITION DESCRIPTION CONFIG INSTALL')
        state = State.WHATEVER
        with open(os.path.join(opkg_target_dir, makefile_name), 'r') as makefile:
            for line in makefile:
                if state == State.WHATEVER:
                    if line == 'define Package/' + package_name + '\n':
                        state = State.DEFINITION
                        continue

                    if line == 'define Package/' + package_name + '/description\n':
                        state = State.DESCRIPTION
                        package.description = ''
                        continue

                    if line == 'define Package/' + package_name + '/config\n':
                        state = State.CONFIG
                        package.config_section = ''
                        continue

                    if line == 'define Package/' + package_name + '/install\n':
                        state = State.INSTALL
                        package.install_section = ''
                        continue

                elif state == State.DESCRIPTION:
                    if line == 'endef\n':
                        state = State.WHATEVER
                        continue

                    package.description += line

                elif state == State.CONFIG:
                    if line == 'endef\n':
                        state = State.WHATEVER
                        continue

                    package.config_section += line

                elif state == State.INSTALL:
                    if line == 'endef\n':
                        state = State.WHATEVER
                        continue

                    package.install_section += line

                elif state == State.DEFINITION:
                    if line == 'endef\n':
                        state = State.WHATEVER
                        continue

                    m = re.match('.*VERSION[\\s]*=[\\s]*([^)]*)\n', line)
                    if m:
                        package.version = m.group(1)
                        continue

                    m = re.match('.*EXTRA_DEPENDS[\\s]*[+]=[\\s]*(\\S*)[\\s]*[(](\\D*)([^)]*)[)].*', line)
                    if m:
                        name = m.group(1)
                        operator = m.group(2)
                        version = m.group(3)
                        if package.installed and name not in package.dependencies:
                            continue

                        if name not in package.dependency_constraints:
                            package.dependency_constraints[name] = list()

                        package.dependency_constraints[name].append(Constraint(operator, version))

                        continue

                else:
                    print('Internal error...', file=sys.stderr)
                    sys.exit(1)


def update_package_versions(packages, build_number):
    if not build_number:
        return
    for package_name in sorted(packages.keys()):
        package = packages[package_name]
        package.version = package.version + '.' + build_number


def update_package_dependency_constraints(target_name, packages):
    for package_name in sorted(packages.keys()):
        package = packages[package_name]
        # Update '=' constraints
        for dependency in sorted(package.dependency_constraints.keys()):
            for dependency_constraint in package.dependency_constraints[dependency]:
                if dependency_constraint.operator == '=':
                    if dependency in packages.keys():
                        dependency_constraint.version = packages[dependency].version
                    elif is_system_package(dependency):
                        dependency_constraint.version = get_system_package_constraint(target_name, dependency)[0].version

        # Add default constraints for dependencies if there are none specified yet.
        for dependency in package.dependencies:
            if dependency not in package.dependency_constraints.keys():
                if is_hub_platform_package(dependency):
                    package.dependency_constraints[dependency] = [Constraint('=', packages[dependency].version)]
                elif is_system_package(dependency):
                    package.dependency_constraints[dependency] = get_system_package_constraint(target_name, dependency)


def unique_list(list_obj):
    unique_list = [] 

    for x in list_obj: 
        if x not in unique_list: 
            unique_list.append(x)

    return unique_list
   

def generate_makefile(package):
    depends_mk = ''
    extra_depends_mk = ''
    conffiles_mk = ''
    config_mk = ''
    for dependency in sorted(package.dependencies,
                             key=lambda name: name if is_hub_platform_package(name) else '~' + name):
        depends_mk += '\tDEPENDS += +{0}\n'.format(dependency)
        for constraint in package.dependency_constraints[dependency]:
            extra_depends_mk += '\tEXTRA_DEPENDS += {0} ({1}{2}),\n'.format(dependency, constraint.operator,
                                                                         constraint.version)
    for conffile in sorted(package.conffiles):
        conffiles_mk += '{0}\n'.format(conffile)

    if conffiles_mk:
        conffiles_mk = '''
define Package/{0}/conffiles
{1}endef
'''.format(package.name, conffiles_mk)

    if package.config_section:
        config_mk = '''
define Package/{0}/config
{1}endef
'''.format(package.name, package.config_section)

    if package.install_section:
        install_mk = package.install_section
    else:
        if package.installed:
            install_mk = '''\t$(INSTALL_DIR) $(1)/
\t$(CP) $(PKG_INSTALL_DIR)/{0}/* $(1)/
'''.format(package.name)
        else:
            install_mk = '\t# Meta package, nothing to install\n'

    package_mk = '''define Package/{0}
\t$(call Package/hub-platform/Default)
\tTITLE={0}
\tVERSION={1}
{3}{4}endef

define Package/{0}/description
{2}endef
{5}{6}
define Package/{0}/install
{7}endef

$(eval $(call BuildPackage,{0}))
'''.format(package.name, package.version, package.description, extra_depends_mk, depends_mk,
           conffiles_mk, config_mk, install_mk)

    return package_mk


def update_makefiles(packages_per_target, opkg_dir):
    for target_name in sorted(packages_per_target.keys()):
        path = os.path.join(opkg_dir, target_name)
        # TODO: handle errors except a 'No such file or directory' one, 'ignore_errors' is the best flag ever :)
        shutil.rmtree(path, ignore_errors=True)

    for target_name in sorted(packages_per_target.keys()):
        opkg_target_dir = os.path.join(opkg_dir, target_name)
        os.makedirs(opkg_target_dir, exist_ok=True)

        packages = packages_per_target[target_name]
        for package_name in sorted(packages.keys()):
            package = packages[package_name]
            package_mk_path = os.path.join(opkg_target_dir, package.name + '.mk')

            if package.installed or package.meta:
                makefile_content = generate_makefile(package)
                open(package_mk_path, 'w').write(makefile_content)
            elif os.path.exists(package_mk_path):
                os.remove(package_mk_path)


def check_makefiles(packages_per_target, opkg_dir):
    target_specific_content = dict()
    result = True
    for target_name in sorted(packages_per_target.keys()):
        opkg_target_dir = os.path.join(opkg_dir, target_name)

        packages = packages_per_target[target_name]
        for package_name in sorted(packages.keys()):
            package = packages[package_name]
            package_mk_path = os.path.join(opkg_target_dir, package.name + '.mk')

            if package.installed or package.meta:
                makefile_content = generate_makefile(package)
                try:
                    actual_content = open(package_mk_path, 'r').read()
                    if actual_content != makefile_content:
                        print('Package makefile', package_mk_path, 'is outdated!', '\nactual_content: ', actual_content, '\nmakefile_content: ', makefile_content,
                              file=sys.stderr)
                        sys.stdout.writelines(difflib.unified_diff(actual_content.splitlines(keepends=True),
                                                                   makefile_content.splitlines(keepends=True),
                                                                   package_mk_path,
                                                                   os.path.join(package_mk_path + '.new')))
                        print('')
                        result &= False

                except IOError:
                    print('Package makefile', package_mk_path, 'is missing!', file=sys.stderr)
                    result &= False

            elif os.path.exists(package_mk_path):
                print('Package makefile', package_mk_path, 'is unnecessary!', file=sys.stderr)
                result &= False

    return result


def main():
    parser = argparse.ArgumentParser(description="Generate or update opkg makefiles for hubPlatform packages or genarate .ipk packages for hubPlatform.")
    parser.add_argument("--action", help="desired action", choices=['check', 'update', 'get_version', 'pack'], default='update')
    parser.add_argument('--toolchain_dirs', help='list of directories containing hub subdirs as root for cmake builds')
    parser.add_argument('--hub_type', help='specify hub type (e.g. h2.1, h2_secure.1, beewave.1)')
    parser.add_argument('--opkg_dir', help='destination for opkg Makefiles (actually hubPlatform/misc/opkg)',
                        required=True)
    parser.add_argument('--build_number', help='add third component to version of packages (e.g. MAJOR.MINOR.BUILD)')
    parser.add_argument('--package', help='if you want to get version of some package you should specify its name')
    args = parser.parse_args()

    if args.action == 'get_version':
        if args.package is None:
            print('If you want to get version of some package you should specify its name!', file=sys.stderr)
            return 1

        if args.hub_type is None:
            print('If you want to get version of some package you should specify its hub type!', file=sys.stderr)
            return 1

        packages = dict()

        hub_type = args.hub_type
        if args.hub_type not in hub_types_map:
            hub_type = 'h2_secure.1' # NOTE: H2 secure packages are used for non-deployable targets

        target_name = hub_types_map[hub_type]
        opkg_target_dir = os.path.join(args.opkg_dir, target_name)
        if os.path.exists(opkg_target_dir):
            parse_existing_makefiles(packages, opkg_target_dir)

        update_package_versions(packages, args.build_number)

        if args.package not in packages.keys():
            print('Package', args.package, 'not found!', file=sys.stderr)
            return 1

        package_version = packages[args.package].version
        print(package_version, end='')
        return 0

    if args.toolchain_dirs is None:
        print('If you want to update or check package makefiles or generate .ipk packages for hubPlatform you should specify toolchain dirs!', file=sys.stderr)
        return 1

    packages_per_target = dict()
    toolchain_dirs = args.toolchain_dirs.split(',')
    
    install_prefixes = list()
    for toolchain_dir in toolchain_dirs:
        for hub_dir in os.listdir( toolchain_dir ):
            hub_dir = os.path.join( toolchain_dir, hub_dir )
            if os.path.isdir( hub_dir ): install_prefixes.append( hub_dir + '/installed' )

    # alongside with proper directories (g150, h2.1,...) Jenkins creates similar directories with '@tmp' sufixes, so we ignore 'em
    install_prefixes = [ i for i in install_prefixes if i.find('@tmp') == -1 ]

    for install_prefix in install_prefixes:
        m = re.search('.+build[/](.+)[/](.*)[/].*', os.path.abspath(install_prefix))
        if m:
            target_name = m.group(1) + "-" + m.group(2) # <toolchain-name>-<hub-type>
        else:
            print('Internal error: failed to get a target name (install_prefix=' + install_prefix + ')...', file=sys.stderr)
            return 1

        if not is_target_name(target_name):
            print('Unsupported target name ' + target_name + ', use the next ones: ', list(sorted(target_names.keys())), file=sys.stderr)
            return 1            

        library_packages = fill_library_packages(system_library_packages, install_prefix)
        unresolved_dependencies, packages = fill_dependencies_and_conffiles(library_packages, install_prefix)

        if unresolved_dependencies:
            print('\nUnresolved dependencies for a ', target_name, ' target :', file=sys.stderr)
            for dependency in sorted(unresolved_dependencies):
                print(dependency, file=sys.stderr)
            print('\nHINT:\n'
                  '  Add hubPlatform libraries to proper install target in CMAKE,\n'
                  '  Add system libraries to misc/scripts/openwrt/pkgdeps.py\n', file=sys.stderr)
            return 1

        if have_cycles(packages):
            print('Cycle detected for a', target_name, 'target')
            return 1

        opkg_target_dir = os.path.join(args.opkg_dir, target_name)
        if os.path.exists(opkg_target_dir):
            parse_existing_makefiles(packages, opkg_target_dir)

        update_meta_packages(target_name, packages)
        if args.action == 'update' and args.build_number:
            print('Specified build version will not be used in generated .mk files. Specify --build_number while packing')
        else:
            update_package_versions(packages, args.build_number)
        update_package_dependency_constraints(target_name, packages)

        packages_per_target[target_name] = packages

        if args.action == 'pack':
            # Prepare build directory
            build_dir = os.path.abspath(os.path.join(install_prefix, os.pardir))
            tmp_dir = os.path.join(build_dir, 'tmp')
            packages_dir = os.path.join(build_dir, 'packages')

            builder = ipk_builder.IpkBuilder(tmp_dir, packages_dir, True)
            for package_name in sorted(packages.keys()):
                package = packages[package_name]
                package.architecture = architecture_names[target_name]

                # fill package data
                package_data_dir = builder.get_pkg_data_dir(package)
                package_src_dir = os.path.abspath(os.path.join(install_prefix, package.name))
                if os.path.isdir(package_src_dir):
                    shutil.copytree(package_src_dir, package_data_dir, symlinks=True)
                builder.create_ipk(package)

    if args.action == 'pack':
        return 0

    if args.action == 'update':
        update_makefiles(packages_per_target, args.opkg_dir)
    elif args.action == 'check':
        if not check_makefiles(packages_per_target, args.opkg_dir):
            print('Refer to https://confluence.mios.com/display/ELFD/Firmware+opkg+makefiles '
                  'for details about the problem', file=sys.stderr)
            return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())

