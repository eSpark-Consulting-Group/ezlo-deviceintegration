#!/usr/bin/env python3

import argparse
import sys
import os
import subprocess


def main(args):
    parser = argparse.ArgumentParser(description="Deploy directory as a snapshot to specified server via sftp")
    parser.add_argument('--identity_file', help='Selects the file from which the identity (private key) '
                                                'for public key authentication is read', required=True)
    parser.add_argument('--deploy_path', help='Prefix to deploy to (e.g. "build/firmware/g150/snapshots/42")', required=True)
    parser.add_argument('--source', help='Deploy everything that is in specified directory', required=True)
    args = parser.parse_args(args)

    sftp_port = 8822
    sftp_user = 'ez_bundle'
    sftp_host = 'dl.mios.com'
    sftp_path = '/linux_firmware'

    source_snapshot_path = os.path.join(os.path.dirname(args.source), os.path.basename(args.deploy_path))

    commands = ''
    deploy_path = os.path.dirname(args.deploy_path)
    for component in deploy_path.split('/'):
        commands += '-mkdir ' + component + '\n'
        commands += 'cd ' + component + '\n'

    commands += '-mkdir ' + os.path.basename(args.deploy_path) + '\n'
    commands += 'put -r ' + source_snapshot_path + '\n'

    try:
        os.rename(args.source, source_snapshot_path)

        res = subprocess.run(['sftp', '-b', '-', '-i', args.identity_file, '-P', str(sftp_port),
                              sftp_user + '@' + sftp_host + ':' + sftp_path], input=commands.encode('ascii'))
    finally:
        os.rename(source_snapshot_path, args.source)

    return res.returncode


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
