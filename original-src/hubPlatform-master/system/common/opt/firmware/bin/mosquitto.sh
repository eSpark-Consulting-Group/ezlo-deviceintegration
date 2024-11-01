#!/bin/sh

log_dir=/var/log/firmware

while [ $# -gt 0 ]
do
    case "$1" in
        -l)
            log_dir="$2"
            shift
            ;;

        *)
            echo "$1 is not an option"
            exit 1
            ;;
    esac
    shift
done

firmware_dir="$( dirname "$( dirname "$( readlink -f "$0" )" )" )"
logfile=$log_dir/mosquitto.log

while true
do
    mkdir -p "$log_dir"
    chmod a+w "$log_dir"
    touch "$logfile"

    {
        echo ""
        echo "**** Mosquitto Restarted at $(date)"
        echo ""
    } >> "$logfile"

    "$firmware_dir"/bin/mosquitto -c "$firmware_dir/etc/mosquitto.conf" >> "$logfile" 2>&1
    sleep 1
done

