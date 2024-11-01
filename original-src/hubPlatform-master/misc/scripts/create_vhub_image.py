#! /usr/bin/env python3

import os
import sys
import argparse
import tempfile
import shutil
import subprocess
from pathlib import Path

### NOTE: docker can't work with absolute paths in Dockerfile
### Some workaround with temp dir in code are necessary

HUB_ROOT_NAME = 'hub_root'

def parse_args():
    parser = argparse.ArgumentParser(
        description='Utility to create docker vhub images')
    parser.add_argument(
            '--packages',
            dest='packages',
            required=True,
            help='Path to installed packages',
            )
    parser.add_argument(
            '--plugins_dir',
            dest='plugins_dir',
            required=True,
            help='Path to plugins deps dir',
            )
    parser.add_argument(
            '--base_image',
            dest='base_image',
            required=False,
            help='Name of the vhub base image',
            )
    parser.add_argument(
            '--tag',
            dest='tag',
            required=False,
            help='Tag for the new image',
            )

    return parser.parse_args()

def check_path(path):
    if not os.path.exists(path):
        raise RuntimeError( 'Path "{}" does not exist'.format(path) )

def find_regular_files(path):
    files = [os.path.join(path,f) for f in os.listdir(path)]
    return [f for f in files if os.path.isfile(f)]

def create_temp_dir(path='/tmp'):
    return tempfile.mkdtemp(dir=path)

def delete_temp_dir(path):
    if not path or not os.path.exists(path):
        return
    try:
        shutil.rmtree(path)
        return True
    except Exception as ex:
        print('Exception occurs: {}'.format(ex), flush=True)
    return False

def copy_content(content_dir, output_dir):
    subprocess.run(['cp -rf {}/* {}'.format(content_dir, output_dir)], shell=True, stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=300)

def is_firmware_package(package_name):
    return package_name.startswith('firmware-addons-') or \
           package_name.startswith('firmware-common-') or \
           package_name.startswith('firmware-core-') or \
           package_name.startswith('firmware-utils-') or \
           package_name.startswith('hub-platform-')

def copy_packages(packages_dir, temp_root):
    package_list = os.listdir(packages_dir)
    print('Copy packages from {} to {}'.format(packages_dir, temp_root), flush=True)
    for package in package_list:
        if not is_firmware_package(package):
            continue

        print('Copy package: {}'.format(package), flush=True)
        copy_content(os.path.join(packages_dir,package), temp_root)

def generate_plugins(plugins_dir, temp_root):
    print('Generating plugins', flush=True)
    plugins_gateways = plugins_dir + '/gateways'
    subprocess.run(['python3', 'dev_utils/device_generation.py'], cwd=plugins_gateways, stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=300)

    print('Building plugins', flush=True)
    subprocess.run(['make'], cwd=plugins_dir, stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=100)

    plugins_packages = plugins_dir + "/build/openwrt/packages"

    temp_dir = None
    try:
        packages = [ p for p in find_regular_files(plugins_packages) if p.endswith('.ipk') ]
        if len(packages) == 0:
            return

        temp_dir = os.path.realpath(create_temp_dir())
        check_path(temp_dir)

        for package in packages:
            subprocess.run(['tar', '-zxf', package, '-C', temp_dir], stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=20)
            subprocess.run(['tar', '-zxf', temp_dir+'/data.tar.gz', '-C', temp_dir], stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=20)

        content_dir = temp_dir+'/opt/firmware/plugins'

        output_dir = temp_root+'/opt/firmware/plugins'
        os.makedirs(output_dir)
        check_path(output_dir)

        print('Copy plugins from {} to {}'.format(content_dir, output_dir), flush=True)
        copy_content(content_dir, output_dir)
        delete_temp_dir(temp_dir)

    except Exception as ex:
        print('Exception occurs: {}'.format(ex), flush=True)
        delete_temp_dir(temp_dir)
        raise RuntimeError( 'Fail to deploy plugins' )

def generate_dockerfile(work_dir, base_image, root_name, command):
    content = ''
    content += '# syntax=docker/dockerfile:1\n'
    content += 'FROM {}\n'.format(base_image)
    content += 'COPY {} /\n'.format(root_name)
    content += 'ENTRYPOINT /bin/sh {} ; /bin/bash'.format(command)
    #content += 'CMD ["/bin/sh", "{}"]'.format(command)

    print('Virtual Hub Dockerfile content', flush=True)
    print('---------------------', flush=True)
    print(content, flush=True)
    print('---------------------', flush=True)

    filename = work_dir + '/Dockerfile'
    with open(filename, 'w') as outfile:
        outfile.write(content)
        outfile.flush()

def create_image(work_dir, tag):
    subprocess.run(['docker', 'build', '--tag', tag, '.'], cwd=work_dir, stdout=sys.stdout, stderr=subprocess.STDOUT, check=True, timeout=900)

def main():
    work_dir = None
    status = -1

    try:
        args = parse_args()

        packages = os.path.realpath(args.packages)
        check_path(packages)
        print('Package dir: {}'.format(packages), flush=True)

        plugins_dir = os.path.realpath(args.plugins_dir)
        check_path(plugins_dir)
        print('Plugins dir: {}'.format(plugins_dir), flush=True)

        work_dir = "/tmp/hub"
        check_path(work_dir)
        print('Temp workdir created: {}'.format(work_dir), flush=True)

        hub_root = os.path.join(work_dir, HUB_ROOT_NAME)
        print('Temp hub root created: {}'.format(hub_root), flush=True)

        generate_plugins(plugins_dir, hub_root)
        copy_packages(packages, hub_root)

        if args.base_image:
            default_cmd = '/usr/sbin/start_services'
            generate_dockerfile(work_dir=work_dir, base_image=args.base_image, root_name=HUB_ROOT_NAME, command=default_cmd)
            create_image(work_dir, args.tag)
            print('New docker image created!', flush=True)

        status = 0
    except Exception as ex:
        print('Exception occurs: {}'.format(ex), flush=True)

    except:
        print('Unknown exception occurs', flush=True)

if __name__ == "__main__":
    exit(main())

