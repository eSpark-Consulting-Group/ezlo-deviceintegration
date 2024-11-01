#!/bin/sh

touch /home/.factorytest_required

log() {
    echo "$(date +%FT%T)" $1
    logger -t "remote_updates" $1
}

log "Remote updater initialization started"

start_remote_updates() {
    updates_root=/home/remote_updates
    log "Configuring DHCP server..."
    $updates_root/bin/setup_dhcp.sh
    sleep 5

    log "Starting remote updates service"
    exec $updates_root/bin/remote_updates --firmware=$updates_root/files/firmware.img --controllers=$updates_root/files/controllers.bin --remote-ip=192.168.123.2 --debug=false
}

start_firmware() {
    rm /home/.factorytest_required
    log "Starting firmware after 10 seconds"
    sleep 10
    firmware_dir=/opt/firmware
    $firmware_dir/bin/start.sh

    sleep 5
    touch /home/.factorytest_required
    exit 0
}

if ip a | grep -Eq ': eth0:.*state UP' ; then
    log "Interface is configured, starting firmware instead"
    start_firmware
    exit 0
fi

log "Waiting 10 seconds to check if interface will be configured"
sleep 10

if ip a | grep -Eq ': eth0:.*state UP' ; then
    log "Interface is configured, starting firmware instead"
    start_firmware
    exit 0
else
    log "Interface is not configured"
    start_remote_updates
fi

