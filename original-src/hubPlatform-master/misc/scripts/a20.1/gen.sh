#!/bin/sh
ver=$1
fw_file=fw-v$ver.fupd

sed -i '$i\echo '$ver' > /etc/firmware_version' /root/fw/bin/update.sh
cd /root/fw/ && tar cfz $fw_file bin
#encrypt
openssl enc -aes-256-cbc -salt -in $fw_file -out $fw_file.enc -pass pass:mySillyPassword
mv -f $fw_file.enc $fw_file
# sign
openssl dgst -sha1 -sign /opt/nma/certs/server.key -out $fw_file.sha1 $fw_file
mv -f $fw_file $fw_file.sha1 /var/www/updates/
ls -lh /var/www/updates/$fw_file*
sync
