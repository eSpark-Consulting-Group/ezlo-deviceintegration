#!/bin/bash
########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# List the Wi-Fi networks
########################################################################################################
# Usage: wf-cmd.sh list
# Returns JSON:
#
# {"ssid":"HA_TEAM", "authorized":true, "connected":true, "open":false, "signalLevel":92, "maxLevel":100 }
# {"ssid":"TrustNet", "authorized":false, "connected":false, "open":false, "signalLevel":80, "maxLevel":100 }
# {"ssid":"HA_GUEST", "authorized":false, "connected":false, "open":false, "signalLevel":88, "maxLevel":100 }
#
#
# Where:
#  ssid - network
#  authorized - already saved in config
#  connected - connected now
#  open - net with open auth
#  signalLevel - signal level
#  maxLevel - max signal level

LogsPath="/var/log"
WifiEventsLog="$LogsPath/wifi.log"

LOG=1
if [ $LOG -eq 1 ]; then
    LOGFILE="$LogsPath/wf-cmd.log"
fi

echo "[$(date)] [touch]" >> $LOGFILE

if [ $# -eq 0 ]; then
    echo "[$(date)] [touch] [no arguments supplied] ERROR" >> $LOGFILE
    exit 1
fi

WPASUPP_DRIVER_SKIP=$(lsusb | grep -e "0bda:b720" | wc -l)
if [ $WPASUPP_DRIVER_SKIP -eq 0 ]; then
    WPA_SUPPLICANT_DRIVER=" -D wext "
else
    WPA_SUPPLICANT_DRIVER=""
fi

function start_wpa_supplicant
{
    ifconfig wlan0 up &> /dev/null
    WPASUPP=$(ps -A | grep wpa_supplicant | wc -l)
    if [ $WPASUPP -eq 0 ]; then
        wpa_supplicant $WPA_SUPPLICANT_DRIVER -i wlan0 -c/etc/wpa_supplicant.conf -d -B -f $WifiEventsLog
    fi
}

if [ $1 == "list" ]; then
    /sbin/iwlist wlan0 scan > /tmp/wf.tmp 2>&1
    #cat /tmp/wf.tmp | grep ESSID | grep -oPn '"\K[^"\047]+(?=["\047])' | awk -F: '{print $2}' > /tmp/list.tmp 2>&1
    grep "ESSID" /tmp/wf.tmp | awk -F: '{print $2}' > /tmp/list0.tmp 2>&1
    sed -e 's/^"//'  -e 's/"$//' /tmp/list0.tmp > /tmp/list.tmp
    # to hex convert
    # echo -n "text" | od -A n -t x1 |sed 's/ //g'
    CONNECTED_TO=$(/sbin/wpa_cli status 2>&1 | grep ^ssid | awk -F= '{print $2}')
    #for var in $ALL
    while read var;
    do
                if [ "$var" == "" ]; then
                    continue
                fi
        if [ $(cat /tmp/wf.tmp | grep -A8 "$var" | grep "Encryption key:off" | wc -l) = "1" ]; then
            OPEN="true"
        else
            OPEN="false"
        fi
        if [ "$CONNECTED_TO" == "$var" ]; then

            CONN_NOW="true"
        else
            CONN_NOW="false"
        fi

        if [ $(cat /etc/wpa_supplicant.conf | grep "\"$var\"" | wc -l) -gt 0 ]; then
           AUTH="true"

        else
            AUTH="false"
        fi
        LEVEL=$(cat /tmp/wf.tmp | egrep 'Quality|ESSID' | grep -A1 -m1 "$var" | awk -F= '{print $3}' | tr -d '\n ' | awk -F/ '{print $1}')
        if [ ! $LEVEL ]; then
            LEVEL=0
        fi
        MAXLEVEL=100
        re='^[0-9]+$'
        if ! [[ $LEVEL =~ $re ]] ; then
            REM='dBm'
            LEVEL=${LEVEL:0:-3}
            if [ $LEVEL -gt -50 ] ; then
                LEVEL=100
            elif [ $LEVEL -gt -55 ] ; then
                LEVEL=90
            elif [ $LEVEL -gt -62 ] ; then
                LEVEL=80
            elif [ $LEVEL -gt -65 ] ; then
                LEVEL=75
            elif [ $LEVEL -gt -68 ] ; then
                LEVEL=70
            elif [ $LEVEL -gt -74 ] ; then
                LEVEL=60
            elif [ $LEVEL -gt -79 ] ; then
                LEVEL=50
            elif [ $LEVEL -gt -83 ] ; then
                LEVEL=30
            elif [ $LEVEL -gt -85 ] ; then
                LEVEL=20
            else
                LEVEL=10
            fi
        fi
        OUT="{\"ssid\":\"$var\", \"authorized\":$AUTH, \"connected\":$CONN_NOW, \"open\":$OPEN, \"signalLevel\":$LEVEL, \"maxLevel\":100 }"
        echo $OUT;
        if [ $LOG -eq 1 ]; then
            echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
        fi
    done < /tmp/list.tmp
        rm /tmp/wf.tmp
        rm /tmp/list0.tmp
        rm /tmp/list.tmp


########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Adding Wi-Fi network configuration
########################################################################################################
# Usage: wf-cmd.sh add [SSID] [KEYPASSWORD]
# If SSID is open-auth network, usage: wf-cmd.sh add [SSID] [KEYPASSWORD]
#
#

elif [ $1 == "add" ]; then
        printf "" > /var/log/wifi.wrongkey 
        printf '\n%.0s' {1..20} >> $WifiEventsLog
    start_wpa_supplicant
    if [ ${#3} -ne 0 ]; then
        KEYSIZE=${#3}
        if [ $KEYSIZE -lt 8 ]; then
          echo "Key password size must be more than 8 characters"
          exit;
        fi
        wpa_passphrase "$2" "$3" >> /etc/wpa_supplicant.conf
    else
        printf "network={\n\tssid=\"$2\"\n\tkey_mgmt=NONE\n}\n\n" >> /etc/wpa_supplicant.conf
    fi

    OUT=$(/sbin/wpa_cli reconfigure)
    echo -e "$OUT\nOK";
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi

########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Adding Wi-Fi hidden network configuration
########################################################################################################
# Usage: wf-cmd.sh add_hidden [SSID] [KEYPASSWORD]
# If hidden SSID is open-auth network, usage: wf-cmd.sh add_hidden [SSID]
#
#

elif [ $1 == "add_hidden" ]; then
        start_wpa_supplicant
    if [ ${#3} -ne 0 ]; then
        KEYSIZE=${#3}
        if [ $KEYSIZE -lt 8 ]; then
          echo "Key password size must be more than 8 characters"
          exit;
        fi
        printf "network={\n\tscan_ssid=1\n\tssid=\"$2\"\n\tpsk=\"$3\"\n}\n\n" >> /etc/wpa_supplicant.conf
    else
        printf "network={\n\tscan_ssid=1\n\tssid=\"$2\"\n\tkey_mgmt=NONE\n}\n\n" >> /etc/wpa_supplicant.conf
    fi
        OUT=$(/sbin/wpa_cli reconfigure)
    printf "$OUT\nOK";
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi

########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Connect to Wi-Fi SSID
########################################################################################################
# Usage: wf-cmd.sh connect [SSID]
# The SSID should be present in config file: wf-cmd.sh add before using
# To check connection result use wf-cmd.sh check SSID after
#

elif [ $1 == "connect" ]; then
    killall -w dhclient &> /dev/null
    start_wpa_supplicant
    NET_SSID_DISCONNECT=$(/sbin/wpa_cli status | grep ^ssid | awk -F= '{print $2}')
    NET_ID_CONNECT=$(/sbin/wpa_cli list_networks | awk -v var=$2 '$0~var{print $1}')
    if [ ${#NET_SSID_DISCONNECT} -eq 0 ]; then
        NET_ID_DISCONNECT=99999
    else
        NET_ID_DISCONNECT=$(/sbin/wpa_cli list_networks | awk -v var=$NET_SSID_DISCONNECT '$0~var{print $1}')
    fi
    #wpa_cli disable_network $NET_ID_DISCONNECT
    #wpa_cli enable_network $NET_ID_CONNECT
    /sbin/wpa_cli disconnect
    /sbin/wpa_cli select_network $NET_ID_CONNECT
    /sbin/wpa_cli save_config
    #wpa_cli reconfigure
    OUT=$(/sbin/wpa_cli reconfigure)
    /sbin/dhclient -nw wlan0 &> /dev/null
    OUT="$OUT\nDisconnected from $NET_SSID_DISCONNECT and connecting to $2"
    echo -e $OUT;
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi

########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Checking the Wi-Fi connection state to SSID
########################################################################################################
# Usage: wf-cmd.sh check [SSID]
# Example: wf-cmd.sh check "HA_TEAM"
# ReturnsJSON:
#  {"ssid":"HA_TEAM", "state":"Connected"}
# Where:
#   state: Not connected - no connection
#              Obtaining IP - Wi-Fi connection
#              Connected
#              Bad password
#
#

elif [ $1 == "check" ]; then
    killall dhclient &> /dev/null
    start_wpa_supplicant
    /sbin/wpa_cli scan &> /dev/null
    STATE=$(/sbin/wpa_cli status | egrep "ssid=$2$|wpa_state=COMPLETED" | wc -l)
     if [ $STATE -gt 1 ]; then
         /sbin/dhclient wlan0 &> /dev/null &
         IPSTATE=$(/sbin/wpa_cli status | grep "ip_address" | wc -l)
     if [ $IPSTATE -gt 0 ]; then
             CONNSTATE="ezlo.local_connection.state.connected"
     else
             CONNSTATE="ezlo.local_connection.state.obtaining_ip"
     fi
    else
        AUTHFAIL=$(tac $WifiEventsLog 2>1 | grep -m 1 "ssid=\"$2\" auth_failures=" | wc -l)
        if [ $AUTHFAIL -ge 1 ]; then
                CONNSTATE="ezlo.local_connection.state.bad_password"
            else
                CONNSTATE="ezlo.local_connection.state.network_is_absent"
        fi
    fi
    OUT="{\"ssid\":\"$2\", \"state\":\"$CONNSTATE\"}"
    echo $OUT;
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi


########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Removing the Wi-Fi SSID from configuration
########################################################################################################
# Usage: wf-cmd.sh remove [SSID]
# Example: wf-cmd.sh remove "HA_TEAM"
#
#

elif [ $1 == "remove" ]; then
    SSID_TO_DELETE="$2"
    SSID_NUMBER_TO_DELETE=$(/sbin/wpa_cli list_networks | awk -v var=$SSID_TO_DELETE '$0~var{print $1}')
    if [[ "${#SSID_NUMBER_TO_DELETE}" -gt 1 ]];
    then
       IFS=$'\n' read -d '' -r -a array <<< "$SSID_NUMBER_TO_DELETE"
       echo -e "[$(date)] Multiple records with ssid \"$SSID_TO_DELETE\" detected - removing all"
       for element in "${array[@]}"
       do
           echo -e "[$(date)] Removing network_id $element"
           /sbin/wpa_cli remove_network $element
       done
    else
        echo -e "[$(date)] Removing network_id $SSID_NUMBER_TO_DELETE"
        /sbin/wpa_cli remove_network $SSID_NUMBER_TO_DELETE
    fi
    /sbin/wpa_cli save
    /sbin/wpa_cli reconfigure
    /sbin/dhclient -x wlan0
    echo -e "[$(date)] Removed $2"
    echo -e "OK";
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi

########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Resetting the wpa_supplicant config
########################################################################################################
# Usage: wf-cmd.sh reset_config
#

elif [ $1 == "reset_config" ]; then
    yes | cp /etc/wpa_supplicant.default /etc/wpa_supplicant.conf
        /sbin/wpa_cli reconfigure
    /sbin/dhclient -x wlan0 &
    OUT="Config reset"
        printf "$OUT\nOK";
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi

########################################################################################################
########################################################################################################
########################################################################################################
########################################################################################################
# Getting type of connection and IP address - WiFi or wired Ethernet
########################################################################################################
# Usage: wf-cmd.sh info
# Returns
# {"media":"radio", "wlanIp":"10.1.1.113", "ethIp":"10.1.1.152"}
# if Wi-Fi
#
#
# and returns
# {"media":"cable", "wlanIp":"10.1.1.113", "ethIp":"10.1.1.152"}
# if ethernet cable
#
#

elif [ $1 == "info" ]; then
    ETHLINK="$(cat /sys/class/net/eth0/carrier)"
    if [ $ETHLINK == 1 ]; then
        TYPE="cable"
    else
        TYPE="radio"
    fi
    IP_WLAN2=""
    IP_ETH2=""
    IP_PUBLIC=""
    WLAN_IP_COUNT=$(ip addr | awk '/inet/ && /wlan0/{sub(/\/.*$/,"",$2); print $2}' | wc -l)
    ETH_IP_COUNT=$(ip addr | awk '/inet/ && /eth0/{sub(/\/.*$/,"",$2); print $2}' | wc -l)
    IP_WLAN=$(ip addr | awk '/inet/ && /wlan0/{sub(/\/.*$/,"",$2); print $2}' | head -1 )
    if [ $WLAN_IP_COUNT -gt 1 ]; then
        IP_WLAN2=$(ip addr | awk '/inet/ && /wlan0/{sub(/\/.*$/,"",$2); print $2}' | tail -1 )
    fi
    IP_ETH=$(ip addr | awk '/inet/ && /eth0/{sub(/\/.*$/,"",$2); print $2}' | head -1 )
    if [ $ETH_IP_COUNT -gt 1 ]; then
        IP_ETH2=$(ip addr | awk '/inet/ && /eth0/{sub(/\/.*$/,"",$2); print $2}' | tail -1 )
    fi
	IP_PUBLIC=$(curl -s checkip.dyndns.org | grep -oE '([0-9]{1,3}\.){3}[0-9]{1,3}')
    OUT="{\"media\":\"$TYPE\", \"wlanIp\":\"$IP_WLAN\", \"ethIp\":\"$IP_ETH\", \"wlanIp2\":\"$IP_WLAN2\", \"ethIp2\":\"$IP_ETH2\", \"public\":\"$IP_PUBLIC\"}"

    printf "$OUT";
    if [ $LOG -eq 1 ]; then
        echo "[$(date)] ["\"$1\" \"$2\" \"$3\""] [local reply]" $OUT >> $LOGFILE
    fi
fi
