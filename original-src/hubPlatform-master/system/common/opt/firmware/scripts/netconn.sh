#!/bin/bash

# Before using
# Update wifi.sh:
# wpa_supplicant add parameter for logging   -f/var/log/wifi.log
#
# Create 2 files:
# /etc/wpa_supplicant.default
# /etc/wpa_supplicant.conf
#
# === begin file ===
#
# ctrl_interface=/var/run/wpa_supplicant
# update_config=1
#
# === end file   ===
#
# Add to rc.local (before exit 0):
# /root/netconn.sh >> /var/log/netconn.log 2>&1 &
#


# Uncomment for debug
#set -x

firmware_dir=/opt/firmware

PING_MAX_RETRIES=10

WPASUPP_DRIVER_SKIP=$(lsusb | grep -e "0bda:b720" | wc -l)
if [ $WPASUPP_DRIVER_SKIP -eq 0 ]; then
    WPA_SUPPLICANT_DRIVER=" -D wext "
else
    WPA_SUPPLICANT_DRIVER=""
fi

function setWifiIndicator {
    $firmware_dir/bin/ha-ledcmd -s "{\"indicator\":\"wifi\",\"setState\":$1}"
    echo $1 > $firmware_dir/etc/wifiFlag
}

function ap_off {
    echo "[$(date)] [DEBUG] Switchting access point OFF"
    $firmware_dir/scripts/ap_off.sh
}

function ap_on {
    echo "[$(date)] [DEBUG] Switchting access point ON"
    $firmware_dir/scripts/ap_on.sh
}

function ha_restart {
    echo "[$(date)] Restarting ha-uid connection to NMA"
    $firmware_dir/bin/ha-ledcmd -s "{\"indicator\":\"ready\",\"setState\":0}"
    sleep 1
    $firmware_dir/bin/ha-infocmd resetConnection 1
}

function eth_on {
    echo "[$(date)] [DEBUG] Switchting ethernet ON"
    setWifiIndicator 0
    if [ -z "$1" ]; then
        $firmware_dir/bin/ha-ledcmd -s "{\"indicator\":\"ready\",\"setState\":0}"
    fi
    killall -w dhclient
    echo "[$(date)] Starting eth0 dhcp procedure"
    dhclient -v -x wlan0
    ifconfig wlan0 down
    local RESULT="$(dhclient -v -nw eth0)"
    ha_restart &
}

function wlan_on {
    echo "[$(date)] [DEBUG] Switchting WiFi ON"
    $firmware_dir/bin/ha-ledcmd -s "{\"indicator\":\"ready\",\"setState\":0}"
    setWifiIndicator 1
    killall -w dhclient
    killall -w wpa_supplicant
    start_wpa_supplicant
    echo "[$(date)] Starting wlan0 dhcp procedure"
    dhclient -v -x eth0
    ifconfig wlan0 up
    local RESULT="$(dhclient -v -nw wlan0)"
    ha_restart &
}

function check_wrong_key {
    AUTH_FAIL=$(tail -n20 /var/log/wifi.log 2>1 | grep " auth_failures=" | wc -l)
    if [ $AUTH_FAIL -gt 1 ]; then
        echo "[$(date)] Stored Wi-Fi key is incorrect. Need to remove wrong key from config"
        SSID_TO_DELETE=$(tail -n20 /var/log/wifi.log 2>1 | grep " auth_failures=" | awk -F'"'  '{print $2}')
        TOTAL_NUMBER_OF_SAVED_NETWORKS=$(cat /etc/wpa_supplicant.conf | grep network | wc -l)
        if [ $TOTAL_NUMBER_OF_SAVED_NETWORKS -gt 1 ]; then
            SSID_NUMBER_TO_DELETE=$(wpa_cli list_networks | grep -e "$SSID_TO_DELETE" | awk '{print $1}')
            wpa_cli remove_network $SSID_NUMBER_TO_DELETE
            wpa_cli save
        fi
        $firmware_dir/bin/ha-ledcmd -a wrongkey
        printf '\n%.0s' {1..20} >> /var/log/wifi.log
        wpa_cli reconfigure
        printf "$SSID_TO_DELETE\n" > /var/log/wifi.wrongkey
        chmod a+rw /var/log/wifi.wrongkey
        echo "[$(date)] wpa supplicant reconfigured"
    fi
}

function wps_on {
    WPASUPP=$(ps -A | grep wpa_supplicant | wc -l)
    if [ $WPASUPP -eq 0 ]; then
        rm /var/run/wpa_supplicant/wlan0
        start_wpa_supplicant
    fi
    echo "[$(date)] Starting on WPS mode"
    $firmware_dir/bin/ha-ledcmd -a wps
    wpa_cli wps_pbc
    sleep 1
    ap_on
}

function pinger {
    # exclude default gateway in AP mode
    DEF="192.168.177.1"
    ipRouteCommand=$(timeout 3 /sbin/ip route)
    echo -e "[$(date)] ***Route*** \n: $ipRouteCommand"
    IP=$( echo $ipRouteCommand | awk '/default/ { print $3 }')
    pingExecution=$(timeout 4 ping -c1 $IP)
    echo -e "***Ping*** \n $pingExecution"
    echo -e "***Netstat*** \n"
    routeInfo=$( netstat -v -r )
    interfacesInfo=$( netstat -v -i)
    programsInfo=$(netstat -tp)
    echo -e "$routeInfo"
    echo -e "$interfacesInfo"
    echo -e "$programsInfo"
    packet_loss=$( echo $pingExecution | grep -oP '\d+(?=% packet loss)' || echo 100)
    if [ "$IP" == "$DEF" ]; then
      echo "Ip == default gateway"
      packet_loss=100
    fi
}

function start_wpa_supplicant {
    WPASUPP=$(ps -A | grep wpa_supplicant | wc -l)
    if [ $WPASUPP -eq 0 ]; then
        echo "[$(date)] Starting wpa_supplicant"
        local RESULT=$(wpa_supplicant $WPA_SUPPLICANT_DRIVER -i wlan0 -c/etc/wpa_supplicant.conf -d -B -f/var/log/wifi.log)
        echo -e "[$(date)] Starting wpa_supplicant. Result: $RESULT"
    else
        echo -e "[$(date)] wpa_supplicant already running - $WPASUPP"
    fi
}

function check_carriers {
    CARRIER_FILE="/sys/class/net/eth0/carrier"
    if [ -s "$CARRIER_FILE" ]; then
        ETHLINK="$(cat $CARRIER_FILE)"
    else
        ETHLINK="0"
    fi
    CARRIER_FILE="/sys/class/net/wlan0/carrier"
    if [ ! $(cat $CARRIER_FILE >/dev/null 2>/dev/null) ]; then
        WLANLINK="0"
    else
        WLANLINK="$(cat $CARRIER_FILE)"
    fi
}

echo "[$(date)] netconn.sh started"
sleep 1 #spread won't accept frequient requests
ap_off
ifconfig wlan0 up&
WLAN=0
while [ $WLAN == 0 ]; do
    IFUP=$(ifconfig | grep wlan0 | wc -l)
    if [ $IFUP -ne 0 ]; then
        WLAN=1
    fi
    sleep 1
done
start_wpa_supplicant
setWifiIndicator 1
check_carriers
CTRY=0
ERRORLED=0
COLUMNDOTS=0
if [ $ETHLINK == 1 ]; then
    eth_on 1
    F=0
else
    F=1
fi
COUNTER=0
while true; do
    check_carriers
    if [ $ETHLINK == 1 ] && [ $F == 1 ]; then
        if [ $COUNTER -gt 3 ]; then
            echo "[$(date)] [DEBUG] Ethernet cable plugged"
            eth_on
            F=0
            COUNTER=0
        else
            echo "[$(date)] [DEBUG] Ethernet cable recheck counter = $COUNTER"
            let "COUNTER++"
            sleep 1
            continue
        fi
    elif [ $ETHLINK == 0 ] && [ $F == 0 ]; then
        F=1
        COUNTER=0
        wlan_on
    fi
    COUNTER=0
    pinger
    if [ $IP ] && [ $packet_loss -lt 50 ]; then
        if [ $COLUMNDOTS -lt 80 ]; then
            printf "."
            let "COLUMNDOTS++"
        else
            printf ".\n"
            date
            COLUMNDOTS=0
        fi
        if [ $ERRORLED == 1 ]; then
            echo "[$(date)]    Route OK. Visual OK alert / Online"
            $firmware_dir/bin/ha-ledcmd -a ok
            check_carriers
            if [ $ETHLINK == 1 ]; then
                $firmware_dir/bin/ha-ledcmd -s "{\"indicator\":\"wifi\",\"setState\":0}"
            fi

            ERRORLED=0
            ap_off
        fi
        CTRY=1
        CONN=1
    else
        echo "[$(date)] Can't connect to ROUTER / Offline"
        if [ $CTRY -gt $PING_MAX_RETRIES ]; then
            if [ $ERRORLED == 0 ]; then
                echo "[$(date)]     visual alert / Offline"
                $firmware_dir/bin/ha-ledcmd -a error
                CTRY=0
                ERRORLED=1
            fi
        fi
        CONN=0
        let "CTRY++"
    fi
    if [ $CONN == 0 ] && [ $ERRORLED == 1 ]; then
        sleep 2
        check_carriers
        if [ $ETHLINK == 1 ]; then
            echo "[$(date)] eth0 cable linked. Turning wifi off..."
            eth_on
        else
            echo "[$(date)] Ethernet cable not plugged. Turning  Wi-Fi on..."
            ap_off
            wlan_on
            if grep -q "network" /etc/wpa_supplicant.conf ; then
                TRY=0
                echo "[$(date)] Trying to connect stored SSID..."
                $firmware_dir/bin/ha-ledcmd -a connecting
                sleep 1
                while [ $TRY -lt 30 ]; do
                    pinger
                    if [ $IP ] && [ $packet_loss -lt 50 ]; then
                        echo "[$(date)] Connected to stored Wi-Fi network / Online"
                        CONN=1
                        TRY=33
                        $firmware_dir/bin/ha-ledcmd -a ok
                        setWifiIndicator 1
                        ap_off
                    else
                        start_wpa_supplicant
                        echo "[$(date)]     still trying to connect to stored SSID..."
                        check_wrong_key
                        let "TRY++"
                        check_carriers
                        if [ $ETHLINK == 1 ]; then
                            TRY=33
                            break
                        fi
                        sleep 2
                    fi
                done
                if [ $TRY -lt 33 ]; then
                    echo "[$(date)] Stored SSID not found..."
                    $firmware_dir/bin/ha-ledcmd -a nossid
                    wpa_cli reconfigure
                    sleep 5
                    TRY=0
                    echo "[$(date)]  Turning on WPS. Trying to connect by WPS..."
                    wps_on
                    while [ $TRY -lt 30 ]; do
                        pinger
                        if [ $IP ] && [ $packet_loss -lt 50 ]; then
                            echo "[$(date)] Wi-Fi ROUTER present. WPS OK / Online"
                            $firmware_dir/bin/ha-ledcmd -a ok
                            setWifiIndicator 1
                            CONN=1
                            TRY=33
                            ap_off
                        else
                            start_wpa_supplicant
                            echo "[$(date)]     still trying to connect by WPS..."
                            let "TRY++"
                            check_carriers
                            if [ $ETHLINK == 1 ]; then
                                TRY=33
                                break
                            fi
                            sleep 2
                        fi
                    done
                fi
            fi
            if ! grep -q "network" /etc/wpa_supplicant.conf ; then
                TRY=0
                echo "[$(date)] Trying to connect by Wi-Fi Protected Setup (WPS)..."
                wps_on
                while [ $TRY -lt 30 ]; do
                    pinger
                    if [ $IP ] && [ $packet_loss -lt 50 ]; then
                        echo "[$(date)] Wi-Fi ROUTER present / Online"
                        CONN=1
                        TRY=33
                        $firmware_dir/bin/ha-ledcmd -a ok
                        setWifiIndicator 1
                        ap_off
                    else
                        start_wpa_supplicant
                        echo "[$(date)]     still trying to connect by WPS..."
                        let "TRY++"
                        check_carriers
                        if [ $ETHLINK == 1 ]; then
                            TRY=33
                            break
                        fi
                        sleep 2
                    fi
                done
            fi
        fi
    fi
    sleep 1
done
