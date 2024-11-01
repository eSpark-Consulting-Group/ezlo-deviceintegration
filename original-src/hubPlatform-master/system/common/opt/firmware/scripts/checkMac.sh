#!/bin/bash
set -x

firmware_dir=/opt/firmware

CONFIG_PATH=/etc/network/interfaces.d/static
MAC=$($firmware_dir/bin/at88util getMac | tr -dc a-f0-9:)
STORED_MAC=$(cat /etc/network/interfaces.d/static | awk '/'$MAC'/ { print $3 }')
MAC_LINE=$(echo $(awk '/hwaddress ether/{ print NR; exit }' $CONFIG_PATH))

if [ ${#MAC} -eq 17 ]; then
    if  [ -z "$MAC_LINE" ]; then
        echo "hwaddress ether "$MAC >> $CONFIG_PATH
    else if [ -z "$STORED_MAC" ]; then
        sed -ie $MAC_LINE'd' $CONFIG_PATH
        echo "hwaddress ether "$MAC >> $CONFIG_PATH
    fi
    fi
fi

