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

firmware_dir=$( dirname "$( dirname "$( readlink -f "$0" )" )" )
ha_dir=$firmware_dir/bin
addons_dir=$ha_dir/addons
services_dir=$firmware_dir/services

script=$(basename "$0")
daemon=${script%.sh}
daemon=${daemon#ha-}
ulimit -n 201572
ulimit -c 9999999

while true
    do
    mkdir -p "$log_dir"
    {
        echo ""
        echo "**** $daemon Restarted at $(date)"
        echo ""
    } >> "$log_dir/ha-$daemon.log"
    if [ -f "$addons_dir/ha-$daemon" ]; then
        "$addons_dir/ha-$daemon" "--logs=$log_dir" >> "$log_dir/ha-$daemon.log" 2>&1
    elif [ -f "$services_dir/$daemon/ha-$daemon" ]; then
        "$services_dir/$daemon/ha-$daemon" "--logs=$log_dir" >> "$log_dir/ha-$daemon.log" 2>&1
    else
        echo "**** $daemon Restarted at $(date)" >> "$log_dir/ha-restart.log"
        "$ha_dir/ha-$daemon" "--logs=$log_dir" >> "$log_dir/ha-$daemon.log" 2>&1
    fi
    sleep 1
done
