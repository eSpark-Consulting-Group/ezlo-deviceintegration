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
logfile=$log_dir/spread.log

while true
do
    mkdir -p "$log_dir"
    chmod a+w "$log_dir"
    touch "$logfile"
    chown spread:spread "$logfile"
    mkdir -m 0755 -p /var/run/spread

    {
        echo ""
        echo "**** Spread Restarted at $(date)"
        echo ""
    } >> "$logfile"
    if [ -x "$firmware_dir/sbin/spread" ]
    then
        "$firmware_dir"/sbin/spread -c "$firmware_dir/etc/spread.conf" >> "$logfile" 2>&1
    else
        spread -c /etc/spread.conf >> "$logfile" 2>&1
    fi
    sleep 1
done
