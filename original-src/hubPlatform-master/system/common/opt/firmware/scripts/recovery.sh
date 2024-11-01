#!/bin/bash
## /etc/rc.local ##
#mkdir /var/run/spread
#nohup /usr/local/sbin/spread > /dev/null 2>&1 &
#ln -s /dev/ttyS3 /dev/stick
#modprobe gpio-sunxi
#. /opt/gpio_init.sh
#. /opt/leds_init.sh
#echo 1 > /sys/class/gpio/export
#echo 3 > /sys/class/gpio/export
#echo out > /sys/class/gpio/gpio3_ph20/direction
#echo 1 > /sys/class/gpio/gpio3_ph20/value
##wifi
#/opt/firmware/scripts/wifi.sh
#nohup /opt/firmware/bin/ha-ledd recovery > /dev/null 2>&1 &
#/opt/firmware/scripts/netconn.sh >> /var/log/netconn.log 2>&1 &
#sleep 10
#/root/recovery.sh >> /root/recovery.log 2>&1 &
#exit 0

# echo >> /etc/fstab
# echo "tmpfs     /tmp/tmpfs     tmpfs     defaults,size=600M     0 0" >> /etc/fstab


FIRMWARE_URL=http://nma.ezlo.com/firmware/newFullFirmware.img
RECOVERY_FLAG=/root/recovery.flag
PATH_TO_FIRMWARE=/tmp/tmpfs/fullFirmware.img
REMOTE_SIZE=0
BOOT_PARTITION=""
ROOT_PARTITION=""
COUNT=1

print(){
    echo "[$(date)]: $1"
}

ledcmd(){
   /opt/firmware/bin/ha-ledcmd $1 $2 > /dev/null 2>&1
}

error(){
    ledcmd -a updateEzloError
    ledcmd -s "{\"indicator\":\"error\",\"setState\":1}"
    print "ERROR: $1"
    sync;
    sleep 5
}

pinger() {
	IP=$(/sbin/ip route | awk '/default/ { print $3 }')
	packet_loss=$(ping -c1 -q nma.ezlo.com | grep -oP '\d+(?=% packet loss)')
}

saveConfig(){
    CONFIG_PATH=/mnt/opt/firmware/etc/firmware.cfg
    SERIAL_LINE=$(echo $(awk '/serial=/{ print NR; exit }' $CONFIG_PATH))
    sed -ie $SERIAL_LINE'd' $CONFIG_PATH
    sed -i $SERIAL_LINE'i\serial='$1 $CONFIG_PATH
}

restoreSerial(){
    print "Getting serial"
    sleep 2
    (mount $BOOT_PARTITION /mnt) || (sleep 3 && mount $BOOT_PARTITION /mnt) ||
        (sync && umount /mnt && restoreSerial) ||
            (error "Getting serial failed"; reboot; exit 1) || exit 1
    SERIAL=$(echo $(cat /mnt/serial.num))
    MAC=$(echo $(cat /mnt/ezloMAC))
    sync
    umount /mnt

    print "Restoring serial and MAC"
    (mount $ROOT_PARTITION /mnt && saveConfig $SERIAL && echo $MAC >> /mnt/etc/network/interfaces.d/static && sync && umount /mnt) ||
        (sleep 2 && sync && umount /mnt && restoreSerial) ||
            (error "restoreSerial failed"; reboot; exit 1) || exit 1
}

forceBootToRecovery(){
    print "Enable force boot to Recovery"
    if [ ! -f $RECOVERY_FLAG ]; then
        echo 1 > $RECOVERY_FLAG
        (mount $BOOT_PARTITION /mnt && cp /mnt/uEnv.txt.recovery /mnt/uEnv.txt && sync && umount /mnt) ||
            (sync && umount /mnt && forceBootToRecovery) ||
                (error "Force boot to Recovery failed"; reboot; exit 1) || exit 1
    fi
    print "Done!"
}

normalBootMode(){
    print "Return normal boot"
    if [ -f $RECOVERY_FLAG ]; then
        rm -f $RECOVERY_FLAG
    fi
    (mount $BOOT_PARTITION /mnt && cp /mnt/uEnv.txt.normal /mnt/uEnv.txt && sync && umount /mnt) ||
        (sleep 2 && sync && umount /mnt && normalBootMode) ||
            (error "Return normal boot failed"; reboot; exit 1) || exit 1
    print "Done!"
}

isNAND() {
    CHECK_PARTITION=$(cat /proc/partitions | grep nand | wc -l)
    if [ "$CHECK_PARTITION" -ne 0 ]; then
        return 0
    else
        return 1
    fi
}

isEnoughFreeSpace() {
    rm -f $PATH_TO_FIRMWARE
	REMOTE_SIZE=$(wget --tries=10 --retry-connrefused --spider $FIRMWARE_URL 2>&1 | grep Length | awk '{print $2}')
	REMOTE_SIZE=$((REMOTE_SIZE/1024))
	FREE_SPACE=$(df -P /tmp/tmpfs/ | tail -1 | awk '{print $4}')
	FREE_SPACE=$((FREE_SPACE-10000))
	if [ "$REMOTE_SIZE" -lt "$FREE_SPACE" ]; then
	    return 0
	else
	    return 1
	fi
}

checkFirmwareSize(){
    LOCAL_SIZE=$(stat -c %s $PATH_TO_FIRMWARE)
    LOCAL_SIZE=$((LOCAL_SIZE/1024))
    if [ "$LOCAL_SIZE" -eq "$REMOTE_SIZE" ]; then
        return 0
	else
	    return 1
	fi
}
# wget -qO- --read-timeout=30 http://nma.ezlo.com/firmware/fullFirmware.img |  openssl enc -d -aes-256-cbc -salt -pass pass:myNandPassword > 1.xz
# xz -dc 1.xz | cmp /dev/nandb
normalUpgrade() {
    print "normal upgrade"
    print "Downloading firmware"
    DOWNLOAD_FIRMWARE="wget -qO- --read-timeout=30 $FIRMWARE_URL > $PATH_TO_FIRMWARE"
	(eval $DOWNLOAD_FIRMWARE && checkFirmwareSize) ||
	    (print "normalUpgrade download error, attempt 2..." && sleep 5 && eval $DOWNLOAD_FIRMWARE && checkFirmwareSize) ||
	        (error "Network problem"; reboot; exit 1) || exit 1

	forceBootToRecovery

	print "Upgrading firmware"
	(openssl enc -d -aes-256-cbc -salt -pass pass:myNandPassword -in $PATH_TO_FIRMWARE | xz -dc | dd of=$ROOT_PARTITION bs=128k) ||
        (error "normalUpgrade failed, rebotting"; reboot; exit 1) || exit 1

    print "Verifying normal upgrade"
    VERIFY=$(openssl enc -d -aes-256-cbc -salt -pass pass:myNandPassword -in $PATH_TO_FIRMWARE | xz -dc | cmp $ROOT_PARTITION - | grep differ | wc -l)
    if [ $COUNT -lt 2 ]; then
        if [ $VERIFY -ne 0 ]; then
            print "Found error, try to reUpgrade"
            let "COUNT++"
            normalUpgrade
        fi
    else
       error "Cyclical reUpgrade"
       exit 1
    fi

	rm $PATH_TO_FIRMWARE
	print "Done!"
}

onLineUpgrade() {
    forceBootToRecovery
    print "on-line upgrade"
    print "Downloading and upgrading firmware"
    $UPGRADE_FIRMWARE=$(wget -qO- --read-timeout=30 $FIRMWARE_URL | openssl enc -d -aes-256-cbc -salt -pass pass:myNandPassword | xz -dc | dd of=$ROOT_PARTITION bs=128k)
	(eval $UPGRADE_FIRMWARE) ||
	    (print "onLineUpgrade error occured, attempt 2..." && sleep 5 && eval $UPGRADE_FIRMWARE) ||
	        (error "onLineUpgrade failed"; reboot; exit 1) || exit 1

    print "Verifying online upgrade"
	VERIFY=$(wget -qO- --read-timeout=30 $FIRMWARE_URL | openssl enc -d -aes-256-cbc -salt -pass pass:myNandPassword -in $PATH_TO_FIRMWARE | xz -dc | cmp $ROOT_PARTITION - | grep differ | wc -l)
    if [ $COUNT -lt 2 ]; then
        if [ $VERIFY -ne 0 ]; then
            print "Found error, try to reUpgrade"
            let "COUNT++"
            onLineUpgrade
        fi
    else
       error "Cyclical reUpgrade"
       exit 1
    fi
	print "Done!"
}

##########################################################
##########################################################
##########################################################

#set -x
print "Recovery v0.1.3 started"

if isNAND; then
    BOOT_PARTITION=/dev/nanda
    ROOT_PARTITION=/dev/nandb
    print "Working with NAND"
else
    BOOT_PARTITION=/dev/mmcblk0p1
    ROOT_PARTITION=/dev/mmcblk0p2
    print "Working with EMMC"
fi

print "Checking internet
 connection"
ledcmd -s "{\"indicator\":\"ready\",\"setState\":0}"
pinger
while [ ! $IP ] || [ $packet_loss -gt 50 ]; do
   pinger
   sleep 1
done

ledcmd -s "{\"indicator\":\"ready\",\"setState\":1}"
sleep 5
print "Confirm recovery procedure"
ledcmd -a confirmRecovery
COFNFIRM=0
BLINKFLAG=0
while [ $COFNFIRM -eq 0 ] ; do
  if [ $BLINKFLAG -eq 0 ]; then
    ledcmd -s "{\"indicator\":\"error\",\"setState\":0}"
    ledcmd -s "{\"indicator\":\"ready\",\"setState\":1}"
    BLINKFLAG=1
  else
    ledcmd -s "{\"indicator\":\"ready\",\"setState\":0}"
    ledcmd -s "{\"indicator\":\"error\",\"setState\":1}"
    BLINKFLAG=0
  fi
  VALUE=$(cat /sys/class/gpio/gpio1_pi13/value)
  if [ $VALUE -eq 0 ] || [ -f $RECOVERY_FLAG ]; then
    COFNFIRM=1
    ledcmd -s "{\"indicator\":\"ready\",\"setState\":1}"
    ledcmd -s "{\"indicator\":\"error\",\"setState\":0}"
  fi
  sleep 0.2
done

print "Confirmed!!"

ledcmd -a updateEzloFirmware

if isEnoughFreeSpace; then
    normalUpgrade
else
    onLineUpgrade
fi

print "Upgrade procedure complete without errors"
ledcmd -a ok
normalBootMode
#restoreSerial
sleep 2
print "Ezlo will now restart"
sync
reboot
