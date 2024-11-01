# Intro
This is a testing email sender utility based on the `hubPlatform/firmware/common/smtp_client` library.

Utility email_sender can be used to manually test `smtp_client` library.

# Testing

## Prerequisites

* Python version 3.8 or higher for tests with SMTP server

* Extract certificates from `certificates.tgz` archive to the hub
* Extract certificates from `certificates.tgz` archive to the desktop

> [!NOTE]
> Test certificates and a script to generate them can be found in the repository:
`hubPlatform/firmware/common/smtp_client/tests/files/certificates`

* Extract test SMTP server from `smtp_server.tgz` directory to the desktop.

> [!NOTE]
> SMTP server can be found in the repository:
`hubPlatform/firmware/common/smtp_client/tests/files/smtp_server`

* Install `email_sender` utility on the hub
```
opkg update
opkg install firmware-dev-email_sender
```

> [!NOTE]
> SMTP client can be tested on amd64 build in the directory: `build/bin/email_sender`

* Create environment variables `CERT_DIR` on the host. NOTE: Use absolute path to the directory
```
export CERT_DIR=/path/to/certificates_dir
```

* Create environment variables `CERT_DIR` on the hub. NOTE: Use absolute path to the directory
```
export CERT_DIR=/path/to/certificates_dir
```

## Tests with SMTP server

### Without encryption, without authentication

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtp://<server-ip>:9920' --encryption=no --from 'sender@email.none' --to 'receiver@email.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### Without encryption, with authentication

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --client-username 'client.username' --client-password 'client.password'
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtp://<server-ip>:9920' --encryption=no --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

Send email with invalid auth (should fail)
```
/opt/firmware/bin/email_sender '--server=smtp://<server-ip>:9920' --encryption=no --username 'invalid.username' --password 'invalid.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### With encryption, without TLS verification, with authentication

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --server-cert $CERT_DIR/server_cert.pem --server-cert-key $CERT_DIR/server_key.pem --client-username 'client.username' --client-password 'client.password'
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --verify-server-hostname=false --verify-server-origin=false --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

Send email with invalid auth (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --verify-server-hostname=false --verify-server-origin=false --username 'invalid.username' --password 'invalid.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### Encrypted client should reject unencrypted server

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --client-username 'client.username' --client-password 'client.password'
```

Send email (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --verify-server-hostname=false --verify-server-origin=false --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### With encryption, with server verification, with authentication

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --server-cert $CERT_DIR/server_cert.pem --server-cert-key $CERT_DIR/server_key.pem --client-username 'client.username' --client-password 'client.password'
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

Send email with invalid auth (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --username 'invalid.username' --password 'invalid.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### Client rejects server with fake certificate

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --server-cert $CERT_DIR/fake_server_cert.pem --server-cert-key $CERT_DIR/fake_server_key.pem --client-username 'client.username' --client-password 'client.password'
```

Send email (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### With encryption, with server and client verification, with authentication

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --server-cert $CERT_DIR/server_cert.pem --server-cert-key $CERT_DIR/server_key.pem --client-ca-cert $CERT_DIR/ca_cert.pem --client-username 'client.username' --client-password 'client.password'
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --client-cert $CERT_DIR/client_cert.pem --client-cert-key $CERT_DIR/client_key.pem --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

Send email with invalid auth (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --username 'invalid.username' --password 'invalid.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

### Server rejects client with fake certificate

Start server
```
cd /path/to/smtp_server
python3 -B -m smtp_server --bind 0.0.0.0 --port 9920 --server-cert $CERT_DIR/server_cert.pem --server-cert-key $CERT_DIR/server_key.pem --client-ca-cert $CERT_DIR/ca_cert.pem --client-username 'client.username' --client-password 'client.password'
```

Send email (should fail)
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --client-cert $CERT_DIR/fake_client_cert.pem --client-cert-key $CERT_DIR/fake_client_key.pem --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

## Optional tests with smtp4dev

> [!NOTE]
> Stop test server if it's running or use another port

Start smtp4dev
```
docker run --rm -it -p 5000:80 -p 9920:25 \
    -e ServerOptions__TlsMode=ImplicitTls \
    -e ServerOptions__TlsCertificate='/server_cert.pem' \
    -e ServerOptions__TlsCertificatePrivateKey='/server_key.pem' \
    -e ServerOptions__AuthenticationRequired='true' \
    -e ServerOptions__SmtpAllowAnyCredentials='false' \
    -e ServerOptions__CredentialsValidationExpression="credentials.Type == 'USERNAME_PASSWORD' && credentials.username == 'client.username' && credentials.password == 'client.password'" \
    -v $CERT_DIR/server_cert.pem:/server_cert.pem \
    -v $CERT_DIR/server_key.pem:/server_key.pem \
    rnwood/smtp4dev:3.6.0-ci20240808131
```

Open smtp4dev webpage in the browser on port 5000
```
http://127.0.0.1:5000
```

Send email
```
/opt/firmware/bin/email_sender '--server=smtps://<server-ip>:9920' --server-ca-cert $CERT_DIR/ca_cert.pem --verify-server-hostname=false --username 'client.username' --password 'client.password' --from 'sender@mail.none' --to 'receiver@mail.none' --topic 'TOPIC' --body 'BODY' --verbose
```

