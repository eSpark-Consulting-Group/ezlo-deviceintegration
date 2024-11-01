#!/usr/bin/env python3

import argparse
import sys
import pexpect

def main():
    parser = argparse.ArgumentParser(description="Remove directory recursively via sftp")
    parser.add_argument('--identity_file', help='Selects the file from which the identity (private key) '
                                                'for public key authentication is read', required=True)
    parser.add_argument('--dry-run', action='store_true', help='Print what will be removed without removing anything')
    parser.add_argument('path', help='Direcory to remove (e.g. linux_firmware/branch/test)')
    args = parser.parse_args()

    sftp_port = 8822
    sftp_user = 'ez_bundle'
    sftp_host = 'dl.mios.com'

    commands = []

    sftp = pexpect.spawn('sftp', args=['-i', args.identity_file, '-P', str(sftp_port), sftp_user + '@' + sftp_host],
                         encoding='ascii')
    sftp.expect('sftp> ')
    stack = [('/'.join(args.path.split('/')[:-1]), args.path.split('/')[-1])]
    while stack:
        back = stack.pop()
        path = back[0]
        pattern = back[1]
        sftp.sendline('ls -l ' + path)
        sftp.expect('sftp> ')
        items = sftp.before.split("\r\n")[1:-1]
        if len(items) == 1 and items[0].startswith("Can't ls"):
            print('Warning: can not stat ' + path)
            continue

        for item in items:
            columns = item.split()
            if len(pattern) > 0 and columns[-1] != pattern:
                continue

            current_path = path + '/' + columns[-1]
            if columns[0][0] == 'd':
                commands.append('rmdir ' + current_path)
                stack.append((current_path, ''))
            else:
                commands.append('rm    ' + current_path)

    commands.reverse()
    if args.dry_run:
        if commands:
            print('Commands:\n' + '\n'.join(commands))
        else:
            print('Nothing to do')
    else:
        for command in commands:
            sftp.sendline(command)
            sftp.expect('sftp> ')

    sftp.sendline('bye')


if __name__ == '__main__':
    sys.exit(main())
