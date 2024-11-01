#!/bin/sh

# Generate self signed certificates with 10 years expiration time

echo "Generate certificate autority..."
echo "Generate the CA private key"
openssl genpkey -algorithm RSA -out ca_key.pem
echo "Generate the CA certificate"
openssl req -new -x509 -key ca_key.pem -subj "/C=US/CN=smtp.ca"  -out ca_cert.pem -days 3650

echo "Generate server certificates..."
echo "Generate server key"
openssl genpkey -algorithm RSA -out server_key.pem
echo "Generate server sertificate sign request (CSR)"
openssl req -new -key server_key.pem -subj "/C=US/CN=smtp.server" -out server_cert.csr
echo "Generate server cerfificate"
openssl x509 -req -in server_cert.csr -CA ca_cert.pem -CAkey ca_key.pem -CAcreateserial -out server_cert.pem -days 3650
rm server_cert.csr

echo "Generate client certificates..."
echo "Generate client key"
openssl genpkey -algorithm RSA -out client_key.pem
echo "Generate client certificate sign request (CSR)"
openssl req -new -key client_key.pem -subj "/C=US/CN=smtp.client" -out client_cert.csr
echo "Generate client cerfificate"
openssl x509 -req -in client_cert.csr -CA ca_cert.pem -CAkey ca_key.pem -CAcreateserial -out client_cert.pem -days 3650
rm client_cert.csr

echo "Generate fake certificate autority..."
echo "Generate fake CA private key"
openssl genpkey -algorithm RSA -out fake_ca_key.pem
echo "Generate fake CA certificate"
openssl req -new -x509 -key fake_ca_key.pem -subj "/C=US/CN=fale.smtp.ca"  -out fake_ca_cert.pem -days 3650

echo "Generate fake server certificates..."
echo "Generate fake server key"
openssl genpkey -algorithm RSA -out fake_server_key.pem
echo "Generate fake server sertificate sign request (CSR)"
openssl req -new -key fake_server_key.pem -subj "/C=US/CN=fake.smtp.server" -out fake_server_cert.csr
echo "Generate fake server cerfificate"
openssl x509 -req -in fake_server_cert.csr -CA fake_ca_cert.pem -CAkey fake_ca_key.pem -CAcreateserial -out fake_server_cert.pem -days 3650
rm fake_server_cert.csr

echo "Generate fake client certificates..."
echo "Generate fake client key"
openssl genpkey -algorithm RSA -out fake_client_key.pem
echo "Generate fake client certificate sign request (CSR)"
openssl req -new -key fake_client_key.pem -subj "/C=US/CN=fake.smtp.client" -out fake_client_cert.csr
echo "Generate fake client cerfificate"
openssl x509 -req -in fake_client_cert.csr -CA fake_ca_cert.pem -CAkey fake_ca_key.pem -CAcreateserial -out fake_client_cert.pem -days 3650
rm fake_client_cert.csr
