#!/bin/sh

SCRIPT_ROOT="$( dirname "$( readlink -f "$0" )" )"

echo "Installing required packages"

echo "src/gz openwrt_packages http://downloads.openwrt.org/releases/19.07.3/packages/arm_cortex-a7_neon-vfpv4/packages" >> /etc/opkg/customfeeds.conf
echo "src/gz openwrt_base http://downloads.openwrt.org/releases/19.07.3/packages/arm_cortex-a7_neon-vfpv4/base" >> /etc/opkg/customfeeds.conf

echo "Updating packages list"
opkg update > /dev/null 2>&1

echo "Installing sshpass package"
opkg install sshpass

echo "Installing service file"
cp $SCRIPT_ROOT/service_files/remote_updates /etc/init.d/

echo "Enabling service file"
/etc/init.d/remote_updates enable

echo "Disabling firmware"
touch /home/.factorytest_required

sync
