#!/bin/sh

turn_off_lte() {
    echo 201 > /sys/class/gpio/export
    echo "out" > /sys/class/gpio/gpio201/direction
    echo 203 > /sys/class/gpio/export
    echo "in" > /sys/class/gpio/gpio203/direction

    PWR_PIN=/sys/class/gpio/gpio201/value
    STATUS_PIN=/sys/class/gpio/gpio203/value

    echo "Wait till a modem gets over it..."
    sleep 35 # get a modem time to "get over it" to guarante the modem has a determine state 

    status=$(cat $STATUS_PIN)
    if [ $status = "0" ]; then
        echo "LTE modem is still powered, power it off..."

        echo 1 > $PWR_PIN
        sleep 1
        echo 0 > $PWR_PIN
    else
        echo "LTE modem is powered off"
    fi
}

turn_off_devices() {
    echo "turn off a zwave chip..."
    # zwave power supply enable pin PL6 - active HIGH 
    echo 358 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio358/direction
    echo 0 > /sys/class/gpio/gpio358/value

    # zwave reset pin PD10 - active LOW
    #echo 106 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio106/direction
    #echo 0 > /sys/class/gpio/gpio106/value

    echo "turn off a zigbee chip..."
    # zigbee reset pin PL10 - active LOW
    echo 362 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio362/direction
    echo 0 > /sys/class/gpio/gpio362/value

    echo "turn off a 2gig chip..."
    # 2GIG reset pin PC2 - active LOW
    echo 66 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio66/direction
    echo 0 > /sys/class/gpio/gpio66/value

    echo "turn off a usb host..."
    # usb host port enable pin PA2 - active HIGH
    echo 2 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio2/direction
    echo 0 > /sys/class/gpio/gpio2/value

    echo "turn off a bluetooth chip..."
    # bluetooth reset pin PG13 (0 - shutdown, 1 - up)
    echo 205 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio205/direction
    echo 0 > /sys/class/gpio/gpio205/value

    echo "turn off a wifi chip..."
    # wifi reset pin PL7 (0 - shutdown, 1 - up)
    #echo 359 > /sys/class/gpio/export
    #echo out > /sys/class/gpio/gpio359/direction
    #echo 0 > /sys/class/gpio/gpio359/value

    echo "turn off lan/eth0 network interface (actually to turn Ethernet port's LEDs)"
    ubus call network.interface.lan down

    echo "turn off battery charger..."
    echo 1 > /sys/class/gpio/gpio352/value

    kill $(pgrep dbus-daemon)
    kill $(pgrep brcm_patchram_plus)
    kill $(pgrep crond)

    /usr/sbin/led_cmd --stop

    # sometimes ha-firmware has no time to issue a poweroff request to an LTE modem,
    # so make it here
    turn_off_lte
}

turn_off_cpu_cores() {
    echo 0 > /sys/devices/system/cpu/cpu3/online
    echo 0 > /sys/devices/system/cpu/cpu2/online
    echo 0 > /sys/devices/system/cpu/cpu1/online

    echo powersave > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
}

/etc/init.d/firmware stop
turn_off_devices
turn_off_cpu_cores
