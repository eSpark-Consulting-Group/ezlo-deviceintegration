#!/bin/bash
# this script performs download specified version of firmware and updates eZLO
# ! ! ! must be run on eZLO ! ! !

POSITIONAL=()

function print_help {
  echo -e "update_fw.sh allows to update your eZLO firmware from custom package from custom server"
  echo -e "Must be run on eZLO only. Main purpose is to test update flow and update package consistency"
  echo -e ""
  echo -e "Option list:"
  echo -e "\t-t|--target <host|ip>\t - host where package located. Assume next path http://<host> relativly to /var/www on server"
  echo -e "\t-e|--envmnt <env>\t - environment where package located. Possible <env> values: dev, qa, stage, rc"
  echo -e "\t-l|--local <local/path>\t - local path where package located"
  echo -e "\t-f|--filename <filename> - package name without extention ( <filename>.fupd, <filename>.fupd.sha1 )"
  echo -e "\t-p|--prefix <prefix>\t - package version non-default prefix. Default: \"fw-v\""
  echo -e "\t-v|--version <version>\t - package version you want to get"
  echo -e "\t-c|--current-version\t - preserver current vertion number"
  echo -e "\t-s|--stop\t\t - does not start daemons"
  echo -e "\t-r|--reboot\t\t - reboots eZLO after update instead of running start.sh"
  echo -e "\t-h|--help\t\t - prints this message"
  echo -e ""
  echo -e "Usage: ./update_fw.sh -t nma-qa.ezlo.com/custom_updates --filename ble-0.34.85.1 -c"
  echo -e "Usage: ./update_fw.sh -t nma-qa.ezlo.com/updates -v 0.34.70 -c"
  echo -e "Usage: ./update_fw.sh -e dev -v 0.34.70 -c -s"
  echo -e ""
  echo -e "eZLO 2018 "
  exit 0
}

PREFIX="fw-v"

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -t|--target)
    TARGET="$2"
    shift # past argument
    shift # past value
    ;;
    -e|--envmnt)
    ENV="$2"
    shift # past argument
    shift # past value
    ;;
    -f|--filename)
    FILENAME="$2"
    shift # past argument
    shift # past value
    ;;
    -l|--local)
    LOCAL="$2"
    shift # past argument
    shift # past value
    ;;
    -p|--prefix)
    PREFIX="$2"
    shift # past argument
    shift # past value
    ;;
    -v|--version)
    VERSION="$2"
    shift # past argument
    shift # past value
    ;;
    -h|--help)
    HELP_NEEDED="YES"
    shift # past argument
    ;;
    -c|--current-version)
    CURRENT_VER=$(cat /etc/firmware_version)
    shift # past argument
    ;;
    -s|--stop)
    STOP="YES"
    shift # past argument
    ;;
    -r|--reboot)
    REBOOT="YES"
    shift # past argument
    ;;
    *)    # unknown option
    print_help
    exit 1
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

if [[ ${HELP_NEEDED} = "YES" ]]
then
  print_help
fi

if [[ ! -z ${ENV} ]]
then
  case $ENV in
    "dev")
    FW_HOST="nma-dev.ezlo.com/updates"
    ;;
    "qa")
    FW_HOST="nma-qa.ezlo.com/updates"
    ;;
    "stage")
    FW_HOST="nma-stage.ezlo.com/updates"
    ;;
    "rc")
    FW_HOST="nma.ezlo.com/updates"
    ;;
    *)
    echo -e "Failed to determine environment ! Exiting..."
    exit 6
    ;;
  esac
  echo -e "Will use \"$FW_HOST\" as a host"
elif [[ ! -z ${TARGET} ]]
then
  FW_HOST="$TARGET"
else [[ ! -z ${LOCAL} ]]
  FW_HOST="${LOCAL}"
fi


if [[ -z ${FW_HOST} ]]
then
  echo -e "Host is mandatory parameter ! Exiting..."
  exit 1
fi

if [[ ! -z ${CURRENT_VER} ]]
then
  echo -e "Preserving current version number \"${CURRENT_VER}\". Requested version \"${VERSION}\""
fi

if [[ ! -z ${REBOOT} ]]
then
  echo -e "eZLO will be rebooted after successful update"
fi

if [[ ! -z ${FILENAME} ]]
then
  echo -e "Will be updated with specified filename - \"$FILENAME.fupd\""
  FW_NAME="${FILENAME}.fupd"
  FW_SHA1="${FILENAME}.fupd.sha1"
elif [[ ! -z ${VERSION} ]]
then
  echo -e "Will be updated with specified version - \"$PREFIX$VERSION.fupd\""
  FW_NAME="${PREFIX}${VERSION}.fupd"
  FW_SHA1="${PREFIX}${VERSION}.fupd.sha1"
else
  echo -e "Can not get update package filename from input parameters ! Exiting..."
  exit 7
fi

echo -e "Prepair to get firmware $FW_NAME from $FW_HOST"

if [[ -z "${LOCAL}" ]]
then
  wget -O /tmp/$FW_NAME --tries=2 --timeout=10 $FW_HOST/$FW_NAME
else
  cp -f $FW_HOST/$FW_NAME /tmp/
fi

if [ ! -s "/tmp/$FW_NAME" ];
then
  echo -e "Getting $FW_HOST/$FW_NAME failed"
  exit 2
fi

if [[ -z "${LOCAL}" ]]
then
  wget -O /tmp/$FW_SHA1 --tries=2 --timeout=10 $FW_HOST/$FW_SHA1
else
  cp -f $FW_HOST/$FW_SHA1 /tmp/
fi

if [ ! -s "/tmp/$FW_SHA1" ];
then
  echo -e "Getting $FW_HOST/$FW_SHA1 failed"
  exit 3
fi

firmware_dir=/opt/firmware
echo -e "Moving firmware packages to $firmware_dir/fw ..."
mkdir -p $firmware_dir/fw/
rm -rf $firmware_dir/fw/* && sync
mv /tmp/$FW_NAME /tmp/$FW_SHA1 $firmware_dir/fw 

echo -e "Verifying firmware..."
RES="$(openssl dgst -sha1 -verify $firmware_dir/certs/public.pem -signature $firmware_dir/fw/$FW_SHA1 $firmware_dir/fw/$FW_NAME > /dev/null 2>&1)"
if [ ! -z "$RES" ];
then
  echo -e "Firmware verification failed with error: $RES"
  exit 4
fi

echo -e "Decrypting firmware..."
RES="$(openssl enc -d -aes-256-cbc -salt -in $firmware_dir/fw/$FW_NAME -out $firmware_dir/fw/firmware.dec -pass pass:mySillyPassword > /dev/null 2>&1)"
if [ ! -z "$RES" ];
then
  echo -e "Firmware decription failed with error: $RES"
  exit 5
fi

echo -e "Extracting firmware..."
cd $firmware_dir/fw && tar xf firmware.dec > /dev/null 2>&1 && sync

echo -e "Updating firmware..."
$firmware_dir/fw/bin/update.sh > /var/log/firmware/update.log 2>&1 && sync

echo -e "Wiping $firmware_dir/fw/*..."
rm -rf $firmware_dir/fw/* && sync

if [[ ! -z ${CURRENT_VER} ]]
then
  echo -e "Preserving current version number \"${CURRENT_VER}\"."
  echo ${CURRENT_VER} > /etc/firmware_version
fi

if [[ ! -z ${REBOOT} ]]
then
  echo -e "Rebooting..."
  reboot
fi

if [[ ${STOP} = "YES" ]]
then
  echo -e "Stopping daemons..."
  $firmware_dir/bin/stop.sh
else
  echo -e "Restarting daemons..."
  $firmware_dir/bin/stop.sh
  sleep 1
  $firmware_dir/bin/start.sh
fi

exit 0
