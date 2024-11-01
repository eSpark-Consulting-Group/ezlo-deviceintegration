# Certificates for component tests

## Generation instructions

### Correct certificates

1. Root
   1. Root key

      ```bash
      openssl genrsa -out root.key 2048
      ```

   2. Root csr

      ```bash
      openssl req -new -key root.key -out root.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=eZLO LTD/OU=Linux Firmware Team/CN=eZLO LTD Test CA"
      ```

   3. Root crt

      ```bash
      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out root.crt -startdate 120815080000Z -enddate 420815090000Z -extensions v3_ca -selfsign -keyfile root.key -infiles root.csr
      ```

2. Test controller key, csr and crt

   ```bash
   openssl req -nodes -newkey rsa:2048 -keyout test_controller.key -out test_controller.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=77777777-7777-7777-7777-777777777777/CN=77777777"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller.crt -startdate 120815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_controller.csr
   ```

4. Test firmware key, csr and crt

   ```bash
   openssl req -nodes -newkey rsa:2048 -keyout test_firmware.key -out test_firmware.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=754f4577-cf15-49e2-ac14-8222f90702cd/CN=00000000"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_firmware.crt -startdate 120815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_firmware.csr
   ```

### Invalid certificates

1. Too old test controller crt

   ```bash
   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_too_old.crt -startdate 120815080000Z -enddate 120815090000Z -cert root.crt -keyfile root.key -infiles test_controller.csr
   ```

2. Too new test controller crt

   ```bash
   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_too_new.crt -startdate 420815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_controller.csr
   ```

3. Wrong uuid in test controller crt

   ```bash
   openssl req -new -key test_controller.key -out test_controller_wrong_uuid.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/CN=77777777"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_wrong_uuid.crt -startdate 120815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_controller_wrong_uuid.csr
   ```

4. Lower serial in test controller crt

   ```bash
   openssl req -new -key test_controller.key -out test_controller_lower_serial.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=77777777-7777-7777-7777-777777777777/CN=########"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_lower_serial.crt -startdate 120815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_controller_lower_serial.csr
   ```

5. Higher serial in test controller crt

   ```bash
   openssl req -new -key test_controller.key -out test_controller_higher_serial.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=77777777-7777-7777-7777-777777777777/CN=XXXXXXXX"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_higher_serial.crt -startdate 120815080000Z -enddate 420815090000Z -cert root.crt -keyfile root.key -infiles test_controller_higher_serial.csr
   ```

5. Test controller crt with wrong root

   1. Wrong root key

      ```bash
      openssl genrsa -out root_wrong.key 2048
      ```

   2. Wrong root csr

      ```bash
      openssl req -new -key root_wrong.key -out root_wrong.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=eZLO LTD/OU=Linux Firmware Team/CN=eZLO LTD Test CA"
      ```

   3. Wrong root crt

      ```bash
      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out root_wrong.crt -startdate 120815080000Z -enddate 420815090000Z -extensions v3_ca -selfsign -keyfile root_wrong.key -infiles root_wrong.csr
      ```

   4. Wrong root test controller crt

      ```bash
      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_root_wrong.crt -startdate 120815080000Z -enddate 420815090000Z -cert root_wrong.crt -keyfile root_wrong.key -infiles test_controller.csr
      ```

6. Test controller crt with too old root

   1. Too old root crt

      ```bash
      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out root_too_old.crt -startdate 120815080000Z -enddate 120815090000Z -extensions v3_ca -selfsign -keyfile root.key -infiles root.csr
      ```

   2. Too old root controller crt

      ```bash
      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_root_too_old.crt -startdate 120815080000Z -enddate 420815090000Z -cert root_too_old.crt -keyfile root.key -infiles test_controller.csr
      ```

7. Test controller crt is root

      ```bash
      openssl genrsa -out test_controller_is_root.key 2048

      openssl req -new -key test_controller_is_root.key -out test_controller_is_root.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=77777777-7777-7777-7777-777777777777/CN=77777777"

      openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_is_root.crt -startdate 120815080000Z -enddate 420815090000Z -selfsign -keyfile test_controller_is_root.key -infiles test_controller_is_root.csr
      ```

8. Test controller crt with other controller crt as parent

      ```bash
   openssl req -nodes -newkey rsa:2048 -keyout test_controller_deep.key -out test_controller_deep.csr -subj "/C=UA/ST=Kharkiv/L=Kharkiv/O=controller/OU=77777777-7777-7777-7777-777777777777/CN=77777777"

   openssl ca -create_serial -config /etc/ssl/openssl.cnf -policy policy_anything -out test_controller_deep.crt -startdate 120815080000Z -enddate 420815090000Z -cert test_controller_higher_serial.crt -keyfile test_controller.key -infiles test_controller_deep.csr
   ```

