#!/bin/bash

#parameters
USER="ubuntu"
PORT="8822"
DEPLOY="0"

function print_help {
  echo -e "update_fw.sh allows to update your eZLO firmware from custom package from custom server"
  echo -e "Must be run on eZLO only. Main purpose is to test update flow and update package consistency"
  echo -e ""
  echo -e "Option list:"
  echo -e "\t-e|--envmnt <env>\t - environment where package located. Possible <env> values: dev, qa, stage, rc"
  echo -e "\t-d|--deploy <env>\t - flag that that allows deploy and tag creation. If not set - will create fwuc packs locally"
  echo -e "\t-v|--version <version>\t - package version you want to get"
  echo -e "\t-H|--hubtype\t - Target hub type. Now supported only \"a20.1\""
  echo -e "\t-u|--user\t\t - NMA server user ( needed for ssh to upload files ). Default: \"ubuntu\""
  echo -e "\t-p|--port\t\t - NMA server port ( needed for ssh to upload files ). Default: \"8822\""
  echo -e "\t-t|--test\t\t - Test flag for parameters review. No actions will be performed"
  echo -e "\t-h|--help\t\t - prints this message"
  echo -e ""
  echo -e "Usage: .../$(basename -- "$0") -e dev -v 0.34.96 -H a20.1"
  echo -e "Usage: .../$(basename -- "$0") -e dev -v 0.34.96 --user root -p 12345 -d --hubtype a20.1 -t"
  echo -e ""
  echo -e "eZLO 2018 "
  exit 0
}

PREFIX="fw-v"

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -d|--deploy)
    DEPLOY="1"
    shift # past argument
    ;;
    -e|--envmnt)
    ENV="$2"
    shift # past argument
    shift # past value
    ;;
    -v|--version)
    VERSION="$2"
    shift # past argument
    shift # past value
    ;;
    -H|--hubtype)
    HUB_TYPE="$2"
    shift # past argument
    shift # past value
    ;;
    -h|--help)
    HELP_NEEDED="YES"
    shift # past argument
    ;;
    -u|--user)
    USER="$2"
    shift # past argument
    shift # past value
    ;;
    -p|--port)
    PORT="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--test)
    TEST="YES"
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

case $ENV in
    dev)
    TARGET="dev"
    SERVER="nma-dev.ezlo.com"
    ;;
    qa)
    TARGET="qa"
    SERVER="nma-qa.ezlo.com"
    ;;
    stage)
    TARGET="stage"
    SERVER="nma-stage.ezlo.com"
    ;;
    rc)
    TARGET="release"
    SERVER="nma.ezlo.com"
    ;;
    *)    # unknown option
    echo -e "Unknown environment passed: $ENV. Terminating..."
    print_help
    exit 2
esac

# exit immidiately if something went wrong and non-zero status returned
set -e

PWD="$(pwd)"
SOURCES_DIR="$(readlink --canonicalize $PWD/../../../)"
BUILD_DIR="$SOURCES_DIR/build"
DEPS_DIR="$SOURCES_DIR/deps/build/arm"
DEPS_LIB="$DEPS_DIR/lib"
ARM_TOOLCHAIN_PATH=/usr/local/toolchains/gcc7.2-arm
RESULT_DIR="$BUILD_DIR/result"

SYSTEM_FILES_DIR="$SOURCES_DIR/system/$HUB_TYPE"

#bash colors for output
green='\033[1;32m'
cyan='\033[1;36m'
red='\033[0;31m'
nc='\033[0m'

if [[ ( -z "$VERSION" ) || ( -z "$ENV" ) ]]
then
    echo -e "${red}Imcomplete parameters list ! Exit...${nc}"
    print_help
    exit 1
fi

echo -e "Parameters Ok: $TARGET-$VERSION -> $USER@$SERVER:$PORT, hub type: $HUB_TYPE"
echo -e "Sources folder:        $SOURCES_DIR"
echo -e "Working folder:        $PWD"
echo -e "Build folder:          $BUILD_DIR"
echo -e "Result folder:         $RESULT_DIR"
echo -e "Target system files:   $SYSTEM_FILES_DIR"

if [ ! -z "$TEST" ]
then
    echo -e "Test flag passed - terminating..."
    exit 0
fi

### this path is needed for backward compatibilty - update.sh must be placed there
mkdir -p $BUILD_DIR/fwuc/bin/
cp -f $SYSTEM_FILES_DIR/update.sh $BUILD_DIR/fwuc/bin/

mkdir -p $BUILD_DIR/fwuc/boot/
mkdir -p $BUILD_DIR/fwuc/root/.ssh/ssh_keys/
mkdir -p $BUILD_DIR/fwuc/etc/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/bin/addons/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/certs/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/etc/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/lib/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/sbin/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/scripts/
mkdir -p $BUILD_DIR/fwuc/opt/firmware/zw-flash/

mkdir -p $RESULT_DIR/

### boot partition staff
cp -f $SYSTEM_FILES_DIR/boot/script.bin $BUILD_DIR/fwuc/boot

### handling ssh staff
cp -rf $SYSTEM_FILES_DIR/root/.ssh/* $BUILD_DIR/fwuc/root/.ssh

### handling etc staff ( system configs, scripts etc. )
cp -rf $SYSTEM_FILES_DIR/etc/* $BUILD_DIR/fwuc/etc/

### handling firmware bin
cp -f $SYSTEM_FILES_DIR/opt/firmware/bin/*.sh $BUILD_DIR/fwuc/opt/firmware/bin/

### handling certificates
cp -rf $SYSTEM_FILES_DIR/opt/firmware/certs/* $BUILD_DIR/fwuc/opt/firmware/certs/

### handling firmware etc
cp -rf $SYSTEM_FILES_DIR/opt/firmware/etc/* $BUILD_DIR/fwuc/opt/firmware/etc/

### handling firmware lib - predownloaded or 3rd party provided
cp -rf $SYSTEM_FILES_DIR/opt/firmware/lib/* $BUILD_DIR/fwuc/opt/firmware/lib/

### handling sbin
cp -f $DEPS_DIR/sbin/spread $BUILD_DIR/fwuc/opt/firmware/sbin/

### handling scripts
cp -rf $SYSTEM_FILES_DIR/opt/firmware/scripts/* $BUILD_DIR/fwuc/opt/firmware/scripts/

### handling zw-flash
cp -rf $SYSTEM_FILES_DIR/opt/firmware/zw-flash/* $BUILD_DIR/fwuc/opt/firmware/zw-flash/

### handling built staff
### daemons and binaries that were built
pushd $BUILD_DIR/bin
cp -f ha-* at88util $BUILD_DIR/fwuc/opt/firmware/bin/
cp -f fb-zwflash $BUILD_DIR/fwuc/opt/firmware/zw-flash/
popd

cp -f $BUILD_DIR/addons/bin/ha-* $BUILD_DIR/fwuc/opt/firmware/bin/addons/
cp -f $BUILD_DIR/lib/libadapter.so $BUILD_DIR/fwuc/opt/firmware/lib/
cp -f $BUILD_DIR/lib/liblmdb.so $BUILD_DIR/fwuc/opt/firmware/lib/

# other
cp -f $DEPS_LIB/libspread.so.3.0.0 \
$DEPS_LIB/libspread-core.so.3.0.0 \
$DEPS_LIB/libtspread-core.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/

ln -sf libspread-core.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libspread-core.so.3
ln -sf libspread-core.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libspread-core.so
ln -sf libspread.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libspread.so.3
ln -sf libspread.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libspread.so
ln -sf libtspread-core.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libtspread-core.so.3
ln -sf libtspread-core.so.3.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libtspread-core.so

cp -f $DEPS_LIB/libcrypto.so.1.0.0 $DEPS_LIB/libssl.so.1.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/
ln -sf libcrypto.so.1.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libcrypto.so
ln -sf libssl.so.1.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libssl.so

cp -f $DEPS_LIB/libcurl.so $DEPS_LIB/libtiny.so $BUILD_DIR/fwuc/opt/firmware/lib/

cp -f $DEPS_LIB/libuv.so.1.0.0 $DEPS_LIB/libwebsockets.so.12 $BUILD_DIR/fwuc/opt/firmware/lib/
ln -sf libuv.so.1.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libuv.so.1
ln -sf libuv.so.1.0.0 $BUILD_DIR/fwuc/opt/firmware/lib/libuv.so
ln -sf libwebsockets.so.12 $BUILD_DIR/fwuc/opt/firmware/lib/libwebsockets.so

cp -f $DEPS_LIB/libz.so.1.2.11 $BUILD_DIR/fwuc/opt/firmware/lib/
ln -sf libz.so.1.2.11 $BUILD_DIR/fwuc/opt/firmware/lib/libz.so.1
ln -sf libz.so.1.2.11 $BUILD_DIR/fwuc/opt/firmware/lib/libz.so

cp -f $ARM_TOOLCHAIN_PATH/arm-linux-gnueabihf/lib/libstdc++.so.6.0.24 $BUILD_DIR/fwuc/opt/firmware/lib/
ln -sf libstdc++.so.6.0.24 $BUILD_DIR/fwuc/opt/firmware/lib/libstdc++.so.6
ln -sf libstdc++.so.6.0.24 $BUILD_DIR/fwuc/opt/firmware/lib/libstdc++.so

cp -f $ARM_TOOLCHAIN_PATH/arm-linux-gnueabihf/lib/libgcc_s.so \
$ARM_TOOLCHAIN_PATH/arm-linux-gnueabihf/lib/libgcc_s.so.1 \
$ARM_TOOLCHAIN_PATH/arm-linux-gnueabihf/lib/libatomic.so.1.2.0 $BUILD_DIR/fwuc/opt/firmware/lib/
ln -sf libatomic.so.1.2.0 $BUILD_DIR/fwuc/opt/firmware/lib/libatomic.so
ln -sf libatomic.so.1.2.0 $BUILD_DIR/fwuc/opt/firmware/lib/libatomic.so.1

echo -e "${green}Generating firmware with version ${cyan}${VERSION}"
sync

fw_file=fw-v$VERSION.fupd

OPENSSL="/usr/lib/ssl1.0/openssl"

if [ ! -e "$OPENSSL" ];
then
    OPENSSL="$(which openssl)"
fi
OPENSSL_VERSION=$($OPENSSL version | awk '{print $2}' | sed 's/[A-Za-z-]*//g')
MAJOR=$(echo $OPENSSL_VERSION | awk -F. '{print $1}')
MINOR=$(echo $OPENSSL_VERSION | awk -F. '{print $2}')

echo "openssl is -> $OPENSSL, version $OPENSSL_VERSION"

if [ $MAJOR -le 1 ] && [ $MINOR -eq 0 ]
then
    echo "openssl is OK"
else
    echo "FAILED: openssl is too new, needed openssl 1.0.x. Otherwise eZLO won't decrypt..."
    echo "USE: sudo apt update; sudo apt install openssl1.0"
    exit 1
fi

sed -i '$i\echo '$VERSION' > /etc/firmware_version' $BUILD_DIR/fwuc/bin/update.sh
chmod +x $BUILD_DIR/fwuc/bin/update.sh

### pack
pushd $BUILD_DIR/fwuc
tar -Pczf $BUILD_DIR/$fw_file *
popd

pushd $BUILD_DIR
### encrypt
$OPENSSL enc -aes-256-cbc -salt -in $fw_file -out $fw_file.enc -pass pass:mySillyPassword
mv -f $fw_file.enc result/$fw_file
### sign
$OPENSSL dgst -sha1 -sign $SOURCES_DIR/misc/cloud/certs/server.key -out $RESULT_DIR/$fw_file.sha1 $RESULT_DIR/$fw_file
popd

if [[ 1 -eq "$DEPLOY" ]];
then
  echo -e "${green}Deploying to server ${cyan}${VERSION}"
  sleep 3
  scp -P ${PORT} $RESULT_DIR/* ${USER}@${SERVER}:/var/www/updates/
  rm -rf $BUILD_DIR
else
  echo -e "Debug mode - skip deploy on server and wiping"
  echo -e "Result is here: $RESULT"
fi

if [[ 1 -eq "$DEPLOY" ]];
then
  echo -e "Creating git tag..."
  git tag ${TARGET}-${VERSION}
  git push origin --tags
else
  echo -e "Debug mode - skip tag creation"
fi

echo -e "${green}Done${nc}"

exit 0
