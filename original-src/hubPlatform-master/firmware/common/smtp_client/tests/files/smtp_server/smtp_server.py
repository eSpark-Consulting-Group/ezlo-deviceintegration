import ssl

import asyncio
from aiosmtpd.smtp import AuthResult, LoginPassword
from aiosmtpd import _get_or_new_eventloop
from aiosmtpd.smtp import SMTP
from aiosmtpd.handlers import Debugging
from contextlib import suppress
from functools import partial
import signal

import os
import sys
import argparse
import time
import logging

#AIOSMTP_LOGGER = 'mail.log' # Defined by the library

logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger('smtp_server')

#log = logging.getLogger(AIOSMTP_LOGGER)

log.setLevel(logging.DEBUG)

class AutoFlushHandler(logging.StreamHandler):
    def emit(self, record):
        super().emit(record)
        self.flush()

# Create a logger
#log = logging.getLogger()
log.setLevel(logging.DEBUG)

if log.hasHandlers():
    log.handlers.clear()

# Add the custom handler
handler = AutoFlushHandler(sys.stdout)
#handler.setLevel(logging.DEBUG)
log.addHandler(handler)



#handler = logging.StreamHandler(sys.stdout)
#handler.setLevel(logging.DEBUG)
##handler.flush = sys.stdout.flush
#log.addHandler(handler)


def parse_args():
    parser = argparse.ArgumentParser(
        description='SMPT server')

    parser.add_argument(
        '--bind',
        dest='bind_addr',
        help='Server bind address: 0.0.0.0 or 127.0.0.1',
        required=True)

    parser.add_argument(
        '--port',
        dest='port',
        help='Server port',
        required=True)

    parser.add_argument(
        '--server-cert-key',
        dest='server_cert_key',
        help='Server certificate key',
        required=False)

    parser.add_argument(
        '--server-cert',
        dest='server_cert',
        help='Server certificate',
        required=False)

    parser.add_argument(
        '--client-ca-cert',
        dest='client_ca_cert',
        help='Client authority certificate',
        required=False)

    parser.add_argument(
        '--client-username',
        dest='client_username',
        help='Client username',
        required=False)

    parser.add_argument(
        '--client-password',
        dest='client_password',
        help='Client password',
        required=False)

    return parser.parse_args()

class Authenticator:
    def __init__(self, login, password):
        self.login = login
        self.password = password

    def __call__(self, server, session, envelope, mechanism, auth_data):
        log.info( 'Authenticator: auth started' )
        fail_nothandled = AuthResult(success=False, handled=False)
        if mechanism not in ("LOGIN", "PLAIN"):
            return fail_nothandled
        if not isinstance(auth_data, LoginPassword):
            return fail_nothandled
        login = auth_data.login.decode()
        password = auth_data.password.decode()

        log.info( 'Authenticator: received login: {}, password: {}'.format( login, password ) )

        if( login == self.login and password == self.password ):
            log.info( 'Authenticator: AUTH_COMPLETED. Login and password are correct' )
            return AuthResult(success=True)

        log.error( 'Authenticator: AUTH_REJECTED' )
        return fail_nothandled

def main():
    args = parse_args()

    enableTLS = False

    if args.server_cert and args.server_cert_key:
        enableTLS = True
        if not os.path.exists( args.server_cert ):
            log.error( 'Server certificate file not found: {}'.format(str(args.server_cert)))
            return -1
        if not os.path.exists( args.server_cert_key ):
            log.error( 'Server certificate key file not found: {}'.format(str(args.server_cert_key)))
            return -1

    if args.client_ca_cert:
        if not os.path.exists( args.client_ca_cert ):
            log.error( 'Client certificate authority file not found: {}'.format(str(args.client_ca_cert)))
            return -1

    smtps_context = None
    if enableTLS:
        smtps_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        smtps_context.load_cert_chain(certfile=args.server_cert, keyfile=args.server_cert_key)

        if args.client_ca_cert:
            smtps_context.load_verify_locations(cafile=args.client_ca_cert)
            smtps_context.verify_mode = ssl.CERT_REQUIRED

    clientCredentialsRequired = args.client_username and args.client_password

    factory = partial(
            SMTP,
            Debugging(),
            enable_SMTPUTF8=True,
            #tls_context=tls_context,
            auth_require_tls=False, # Allow login/password authentication. SSL initialized independently (not via STARTTLS)
            auth_required=clientCredentialsRequired,
            authenticator=Authenticator(login=args.client_username, password=args.client_password),
        )

    loop = _get_or_new_eventloop()

    loop.set_debug(enabled=True)

    log.debug("Attempting to start server on %s:%s", args.bind_addr, args.port)
    server = server_loop = None
    try:
        server = loop.create_server(
            factory, host=args.bind_addr, port=args.port, ssl=smtps_context
        )
        server_loop = loop.run_until_complete(server)
    except RuntimeError:
        raise
    log.debug(f"server_loop = {server_loop}")
    log.info("SERVER_STARTED on %s:%s", args.bind_addr, args.port)

    with suppress(NotImplementedError):
        loop.add_signal_handler(signal.SIGINT, loop.stop)

    log.debug("Starting asyncio loop")
    with suppress(KeyboardInterrupt):
        loop.run_forever()
    server_loop.close()
    log.debug("Completed asyncio loop")
    loop.run_until_complete(server_loop.wait_closed())
    loop.close()

if __name__ == "__main__":
    main()

