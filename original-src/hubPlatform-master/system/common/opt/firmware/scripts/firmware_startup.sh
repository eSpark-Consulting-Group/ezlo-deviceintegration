#!/bin/sh

# Firmware start up script - runs once during boot process

echo "Running firmware start up script after reboot..."

firmware_dir=/opt/firmware
ha_dir=firmware_dir/bin
log_dir=/var/log/firmware
mkdir -p $log_dir

echo 30 > /proc/sys/kernel/panic

mkdir -p $firmware_dir/cores
echo "$firmware_dir/cores/core.%s.%e" > /proc/sys/kernel/core_pattern

# next lines create hubtype file for firmware if it does not exist
if [ ! -s /etc/hubtype ];
then
  echo "File /etc/hubtype does not exist."
  if [ -x "$firmware_dir/bin/at88util" ]
  then
      HUBTYPE="$($firmware_dir/bin/at88util -t)"
      echo "Creating with deduced type - $HUBTYPE"
  else
      echo "Creating with default type - h2.1"
      HUBTYPE='h2.1'
  fi
  echo "$HUBTYPE" > /etc/hubtype
fi

hub_a20d1_startup(){
  #fix empty modules.dep.bin
  depmod

  # Generate the SSH keys if non-existent
  test -f /etc/ssh/ssh_host_dsa_key || dpkg-reconfigure openssh-server

  # spread default run directory
  mkdir -p /run/spread/
  ln -sf /run /var/run

  modprobe gpio-sunxi
  
  # export SW4 button pin
  echo 1 > /sys/class/gpio/export
  echo both > /sys/class/gpio/gpio1_pi13/edge
  
  # export stick reset pin
  echo 3 > /sys/class/gpio/export
  echo out > /sys/class/gpio/gpio3_ph20/direction
  echo 1 > /sys/class/gpio/gpio3_ph20/value
  
  ln -sf /dev/ttyS3 /dev/stick
  
  /opt/finishUpdate.sh || echo "finishUpdate.sh not found"
  (cd $firmware_dir/zw-flash/ && $firmware_dir/zw-flash/fb-zwflash) || echo "z-wave stick error"
  
  #wifi
  $firmware_dir/scripts/wifi.sh
  
  #place fb here
  #/root/autoSerial.sh >> /root/autoSerial.log 2>&1
  #$firmware_dir/scripts/netconn.sh >> $log_dir/netconn.log 2>&1 &
  $firmware_dir/scripts/checkMac.sh
  resize2fs /dev/mmcblk0p2
}

read -r HUBTYPE < /etc/hubtype
echo "Running on $HUBTYPE hub..."

if [ "$HUBTYPE" = "a20.1" ]
then
  hub_a20d1_startup
elif [ "$HUBTYPE" = "h2.1" ]
then
  echo "No need in this script on h2.1"
  exit 1
fi

echo "Prepair to start daemons..."
echo "Serial $($firmware_dir/bin/at88util -s)"
sleep 2

$firmware_dir/bin/start.sh

exit 0
