#!/bin/bash

# Print all executed commands. For Debug only.
#set -x

echo "Trying deploy script..."
for arg in "$@"
do
	#echo "${arg}"
	PARAM=`echo ${arg} | awk -F= '{print $1}'`
	VALUE=`echo ${arg} | awk -F= '{print $2}'`
	echo "export ${PARAM}=${VALUE}"
	declare "${PARAM}=${VALUE}"
done

SOURCES_ROOT=$(dirname $(realpath $0) )/../../

if [ -z "$HUB_TYPE" ];
then
    echo "Hub type is not specified. Terminating..."
    exit 1
fi

if [ -z "$BUILD_DIR" ];
then
    echo "Build directory is not specified. Terminating..."
    exit 2
fi

if [ -z "$TOOLCHAIN_PATH" ];
then
    echo "Toolchain path is not specified. Terminating..."
    exit 3
fi

if [ -z "$CLOUD_ENVIRONMENT" ];
then
    echo "CLOUD_ENVIRONMENT is not specified. Defaulting to qa..."
    CLOUD_ENVIRONMENT=qa
fi

if [ -z "$WITH_BLE" ];
then
    echo "WITH_BLE is not defined and will be skipped..."
    WITH_BLE=0
fi

if [ -z "$WITH_ZIGBEE" ];
then
    echo "WITH_ZIGBEE is not defined and will be skipped..."
    WITH_ZIGBEE=0
fi

if [ -z "$WITH_2GIG" ];
then
    echo "WITH_2GIG is not defined and will be skipped..."
    WITH_2GIG=0
fi

if [ -z "$TOOLCHAIN_TARGET_NAME" ];
then
    echo "Toolchain target name is not specified. Terminating..."
    exit 4
fi

if [ -z "$CUSTOM_PACKAGES" ];
then
    echo "CUSTOM_PACKAGES is not defined and will be skipped..."
    CUSTOM_PACKAGES=""
fi

case "$HUB_TYPE" in
    "a20.1" | "h2.1" | "h2_secure.1" )
        SSHPASS=`which sshpass`
        if [ -z "$SSHPASS" ];
        then
            echo "You must install \"sshpass\" package first. Terminating..."
            exit 7
        fi

        HUB_PASSWORD="JY~ljhC*Db9kLg#M"
        if [ ! -z "$PASSWORD" ]
        then
            HUB_PASSWORD="$PASSWORD"
            echo "Hub password is passed via parameters: $HUB_PASSWORD"
        fi

        if [ -z "$HUB_IP" ];
        then
            echo "HUB_IP variable is not specified. Terminating..."
            exit 5
        fi

        if [ -z "$HUB_PORT" ];
        then
            HUB_PORT=22
        fi

        HUB_USER="root"
        HUB_ARCH="arm"

        DEPLOY_DIR="/opt/firmware"
        ;;

    "g150" | "g450" | "g550" )
        SSHPASS=`which sshpass`
        if [ -z "$SSHPASS" ];
        then
            echo "You must install \"sshpass\" package first. Terminating..."
            exit 7
        fi

        HUB_PASSWORD=""
        if [ ! -z "$PASSWORD" ]
        then
            HUB_PASSWORD="$PASSWORD"
            echo "Hub password is passed via parameters: $HUB_PASSWORD"
        fi

        if [ -z "$HUB_IP" ];
        then
            echo "HUB_IP variable is not specified. Terminating..."
            exit 5
        fi

        if [ -z "$HUB_PORT" ];
        then
            HUB_PORT=22
        fi

        HUB_USER="root"
        HUB_ARCH="mips"

        DEPLOY_DIR="/opt/firmware"
        ;;

    "local" )
        if [ -z "$LOCAL_DEPLOY_PATH" ]
        then
            LOCAL_DEPLOY_PATH="${HOME}/.hubPlatform/opt/firmware"
            echo "Using default local deploy path: $LOCAL_DEPLOY_PATH"
        fi

        if [ -z "$LOG_DIR" ]
        then
            LOG_DIR="${HOME}/.hubPlatform/var/log"
            echo "Using default log directory: $LOG_DIR"
        fi

        HUB_ARCH="amd64"

        DEPLOY_DIR=$LOCAL_DEPLOY_PATH
        ;;

    *)
        echo "Invalid hub type detected: $HUB_TYPE"
        exit 6
        ;;
esac

echo "Will perform deploy for \"$HUB_TYPE ( $HUB_ARCH )\""

DEPS_ROOT="$SOURCES_ROOT/deps"
BUILD_BIN_ROOT="$BUILD_DIR/bin"
BUILD_LIB_ROOT="$BUILD_DIR/lib"
ADDONS_BIN_ROOT="$BUILD_DIR/addons/bin"
PLUGINS_ROOT="$DEPS_ROOT/HubPlugins"
PLUGINS_PACKAGES="$PLUGINS_ROOT/build/openwrt/packages"
PLUGINS_GATEWAYS_ROOT="$PLUGINS_ROOT/gateways"
PLUGINS_FUNCTIONAL_UNITS_ROOT="$PLUGINS_ROOT/functional_units"
CUSTOM_PACKAGES_ROOT="$BUILD_DIR/custom_packages"

SYSTEM_ROOT="$SOURCES_ROOT/system"
SYSTEM_COMMON_ROOT="$SOURCES_ROOT/system/common"
TOOLCHAIN_SYSTEM_PATH="$TOOLCHAIN_PATH/$TOOLCHAIN_TARGET_NAME"
DEPS_INST_DIR="$TOOLCHAIN_PATH/usr"
INSTALL_ROOT="$DEPS_INST_DIR"
LIBS_ROOT="$TOOLCHAIN_PATH/usr/lib"

DEPLOY_SYSROOT="$(mktemp -d)"
DEPLOY_SYSROOT_OPT="$DEPLOY_SYSROOT/opt"
DEPLOY_SYSROOT_FIRMWARE="$DEPLOY_SYSROOT_OPT/firmware"
DEPLOY_SYSROOT_ETC_INITD="$DEPLOY_SYSROOT/etc/init.d"

function eval_cmd() {
    case "$HUB_TYPE" in
        "a20.1" | "h2.1" | "h2_secure.1" | "g150" | "g450" | "g550" )
            local SSH_CMD="sshpass -p $HUB_PASSWORD ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o KexAlgorithms=+diffie-hellman-group1-sha1 -T $HUB_USER@$HUB_IP -p $HUB_PORT '$1'"
            eval $SSH_CMD
            ;;

        "local" )
            eval $1
            ;;

        *)
            echo "Invalid hub type detected: $HUB_TYPE"
            exit 6
            ;;
    esac
}

function sysroot_make_folders() {
    echo "Creating folder structure..."

    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/bin/addons/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/lib/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/sbin/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/certs/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/metrics/certs/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/config/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/default/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/cores/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/etc/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/etc/custom_packages/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/scripts/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/zw-flash/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/plugins/
    mkdir -p $DEPLOY_SYSROOT_FIRMWARE/services/
    mkdir -p $DEPLOY_SYSROOT_ETC_INITD/

    case "$HUB_TYPE" in
        "local" )
            mkdir -p $DEPLOY_SYSROOT_FIRMWARE/data/
            ;;

        *)
            ln -sf /home/data $DEPLOY_SYSROOT_FIRMWARE/data
            ;;
    esac

    rm -f $DEPLOY_SYSROOT_FIRMWARE/bin/*
    rm -f $DEPLOY_SYSROOT_FIRMWARE/lib/*
    rm -f $DEPLOY_SYSROOT_FIRMWARE/sbin/*
    rm -f $DEPLOY_SYSROOT_FIRMWARE/bin/addons/*
    rm -rf $DEPLOY_SYSROOT_FIRMWARE/plugins/*
    rm -rf $DEPLOY_SYSROOT_FIRMWARE/services/*
    rm -rf $DEPLOY_SYSROOT_ETC_INITD/*
}

function sysroot_wipe_deploy_destination() {
    echo "Wiping destination folders..."
    eval_cmd "rm -f $DEPLOY_DIR/bin/*"
    eval_cmd "rm -f $DEPLOY_DIR/lib/*"
    eval_cmd "rm -f $DEPLOY_DIR/sbin/*"
    eval_cmd "rm -f $DEPLOY_DIR/bin/addons/*"
    eval_cmd "rm -f $DEPLOY_DIR/services/*"
    if [[ ${HUB_TYPE} = local ]] && [[ ! -L ${DEPLOY_DIR}/plugins ]];
    then
        eval_cmd "rm -rf $DEPLOY_DIR/plugins/*"
    fi
}

function sysroot_perform_permissions() {
    echo "Setting up sysroot permissions..."
    chmod -fR a+x $DEPLOY_SYSROOT_FIRMWARE/bin/* $DEPLOY_SYSROOT_FIRMWARE/sbin/* $DEPLOY_SYSROOT_FIRMWARE/lib/* $DEPLOY_SYSROOT_FIRMWARE/bin/addons/* $(dirname $DEPLOY_SYSROOT_FIRMWARE)/finishUpdate.sh
}

function sysroot_perform_deploy() {
    echo "Performing deploy..."
    local deployFirmwareDir="${DEPLOY_SYSROOT}/opt/firmware"

    rm "${deployFirmwareDir}/etc/cloud.cfg"
    rm "${deployFirmwareDir}/etc/zwave.cfg"
    rm "${deployFirmwareDir}/zw-flash/*.txt"
    rm "${deployFirmwareDir}/zw-flash/*.hex"
    rm "${deployFirmwareDir}/etc/custom_packages/*.txt"

    if [ "$WITH_BLE" = "0" ];
    then
        rm "${deployFirmwareDir}/bin/addons/ha-bluetoothd"
    fi

    if [ "$WITH_ZIGBEE" = "0" ];
    then
        rm "${deployFirmwareDir}/bin/addons/ha-zigbeed"
    fi

    if [ "$WITH_2GIG" = "0" ];
    then
        rm "${deployFirmwareDir}/bin/addons/ha-2gigd"
    fi

    if [ "$HUB_TYPE" = "local" ];
    then
        rm "${deployFirmwareDir}/etc/firmware.cfg"
        mkdir -p "$DEPLOY_DIR"
        pushd "$DEPLOY_SYSROOT_OPT" || exit
        tar -zc . | tar -zxC "$(dirname "$DEPLOY_DIR")"
        echo "Checking for cloud.cfg..."
        eval_cmd "if [[ ! -e $DEPLOY_DIR/etc/cloud.cfg ]]; then ln -sf cloud.cfg.$CLOUD_ENVIRONMENT $DEPLOY_DIR/etc/cloud.cfg; fi"
    else
        pushd "$DEPLOY_SYSROOT" || exit
        if [ -n "$(command -v rsync)" ] && [ -n "$(eval_cmd "command -v rsync")" ];
        then
            # In order to use rsync for deploy you need rsync on both your pc and on controller.
            # On G150 rsync makes subsequent deploys up to ~10x faster
            # You can install rsync on G150 with command:
            # opkg install http://archive.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/packages/packages/rsync_3.1.1-2_ramips_24kec.ipk
            # And on H2 with command (deploy on H2 is already fast, so no need in rsync):
            # opkg install http://archive.openwrt.org/releases/packages-18.06/arm_cortex-a7_neon-vfpv4/packages/rsync_3.1.3-1_arm_cortex-a7_neon-vfpv4.ipk

            echo "Using rsync..."
            sshpass -p "$HUB_PASSWORD" rsync -crlpzzvPh -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o KexAlgorithms=+diffie-hellman-group1-sha1 -p $HUB_PORT" ./etc/ "$HUB_USER@$HUB_IP:/etc"
            sshpass -p "$HUB_PASSWORD" rsync -crlpzzvPh --delete -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o KexAlgorithms=+diffie-hellman-group1-sha1 -p $HUB_PORT" ./opt/ "$HUB_USER@$HUB_IP:/opt"
        else
            echo "Using tar over ssh..."
            tar --owner=root --group=root -zcf - . | sshpass -p "$HUB_PASSWORD" ssh -o StrictHostKeyChecking=no -o KexAlgorithms=+diffie-hellman-group1-sha1 -o UserKnownHostsFile=/dev/null -p $HUB_PORT "$HUB_USER@$HUB_IP" "cd /; tar -zxf -"
        fi
    fi
    popd || exit

    echo "Checking for zwave.cfg..."
    eval_cmd "if [[ ! -e $DEPLOY_DIR/etc/zwave.cfg ]]; then ln -sf zwave.cfg.$ZWAVE_REGION $DEPLOY_DIR/etc/zwave.cfg; fi"
}

function sysroot_copy_configs() { #args: 1.src 2.dst
    local src=$1
    local dst=$2

    for cfgName in $src; do
        if [[ "$cfgName" == *".cfg"* ]]; then
            cp -p $cfgName $dst
        fi
    done
}

function sysroot_copy_libs() { #args: 1.src 2.dst
    local src=$1
    local dst=$2

    for libName in $src; do
        if [[ "$libName" == *".so"* ]]; then
            if [[ -L "$libName" ]]; then
                local libRealPath=$(readlink $libName)
                ln -sf $(basename $libRealPath) $dst/$(basename $libName)
            else
                cp -p $libName $dst
            fi
        fi
    done
}

function sysroot_copy_all_libs() {
    sysroot_copy_libs "$BUILD_LIB_ROOT/*" "$DEPLOY_SYSROOT_FIRMWARE/lib"
    sysroot_copy_libs "$BUILD_DIR/x264/lib/*" "$DEPLOY_SYSROOT_FIRMWARE/lib"
    sysroot_copy_libs "$BUILD_DIR/ffmpeg/lib/*" "$DEPLOY_SYSROOT_FIRMWARE/lib"
}

function sysroot_copy_services() {
    cp -rpf "$BUILD_DIR/services/." "$DEPLOY_SYSROOT_FIRMWARE/services"
#    for service in "$BUILD_DIR/services/*"; do
#        echo "---DEBUG: service:  $service"
#        cp -rp "$service" "$DEPLOY_SYSROOT_FIRMWARE/services/"
#    done
}

function sysroot_copy_firmware() {
    echo "Copying firmware files..."

    cp -prf $BUILD_BIN_ROOT/* $DEPLOY_SYSROOT_FIRMWARE/bin

    pushd $DEPLOY_SYSROOT_FIRMWARE/bin
    for file in $(find . -maxdepth 1 -type f \( -name "ha-*" ! -name "ha-*cmd" \)) ; do echo "$file"; if [ ! -f ${file}.sh ]; then ln -sf hz-daemon.sh ${file}.sh ; fi ; done ;
    popd

    sysroot_copy_all_libs
    sysroot_copy_services

    cp -prf $ADDONS_BIN_ROOT/* $DEPLOY_SYSROOT_FIRMWARE/bin/addons
    cp -prf $SYSTEM_COMMON_ROOT/opt/* $DEPLOY_SYSROOT_OPT
    cp -prf $SYSTEM_COMMON_ROOT/etc/init.d/* $DEPLOY_SYSROOT_ETC_INITD
    cp -prf $SYSTEM_COMMON_ROOT/service_files/* $DEPLOY_SYSROOT_ETC_INITD
    cp -prf $SOURCES_ROOT/firmware/core/metrics/metrics_draft_certs/* $DEPLOY_SYSROOT_FIRMWARE/metrics/certs/
    cp -prf $SYSTEM_COMMON_ROOT/opt/firmware/etc/custom_packages/* $DEPLOY_SYSROOT_FIRMWARE/etc/custom_packages/
    if [ -d "$SYSTEM_ROOT/$HUB_TYPE" ];
    then
        cp -prf $SYSTEM_ROOT/$HUB_TYPE/* $DEPLOY_SYSROOT
    fi

    rm $DEPLOY_SYSROOT_FIRMWARE/etc/*.cfg.in
    sysroot_copy_configs "$BUILD_DIR/generated/system/common/opt/firmware/etc/*.cfg" "$DEPLOY_SYSROOT_FIRMWARE/etc"

    touch "$DEPLOY_SYSROOT_FIRMWARE/default/.default_packages_installed"
}

function sysroot_copy_toolchain_files() {
    echo "Copying toolchain files..."

    cp -p $BUILD_LIB_ROOT/libx264.so* $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $BUILD_LIB_ROOT/libav*.so* $DEPLOY_SYSROOT_FIRMWARE/lib/

    cp -p $INSTALL_ROOT/sbin/spread $DEPLOY_SYSROOT_FIRMWARE/sbin/
    cp -p $LIBS_ROOT/libspread.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/libspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/libtspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/

    ln -sf libspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libspread-core.so.3
    ln -sf libspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libspread-core.so
    ln -sf libspread.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libspread.so.3
    ln -sf libspread.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libspread.so
    ln -sf libtspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libtspread-core.so.3
    ln -sf libtspread-core.so.3.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libtspread-core.so

    cp -p $LIBS_ROOT/libcrypto.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/libssl.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libcrypto.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libcrypto.so
    ln -sf libssl.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libssl.so

    cp -p $LIBS_ROOT/libcurl.so $DEPLOY_SYSROOT_FIRMWARE/lib/

    cp -p $LIBS_ROOT/libuv.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/

    ln -sf libuv.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libuv.so.1
    ln -sf libuv.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libuv.so
    ln -sf libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/libwebsockets.so

    cp -p $LIBS_ROOT/libz.so.1.2.11 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libz.so.1.2.11 $DEPLOY_SYSROOT_FIRMWARE/lib/libz.so.1
    ln -sf libz.so.1.2.11 $DEPLOY_SYSROOT_FIRMWARE/lib/libz.so

    cp -p $LIBS_ROOT/libmongoose.so $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/liblmdb.so $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/liblua.so $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $LIBS_ROOT/libminilzo2.so $DEPLOY_SYSROOT_FIRMWARE/lib/

    cp -p $LIBS_ROOT/libthreadutil.so.6.0.4 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libthreadutil.so.6.0.4 $DEPLOY_SYSROOT_FIRMWARE/lib/libthreadutil.so.6
    ln -sf libthreadutil.so.6.0.4 $DEPLOY_SYSROOT_FIRMWARE/lib/libthreadutil.so

    cp -p $LIBS_ROOT/libupnp.so.6.3.3 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libupnp.so.6.3.3 $DEPLOY_SYSROOT_FIRMWARE/lib/libupnp.so.6
    ln -sf libupnp.so.6.3.3 $DEPLOY_SYSROOT_FIRMWARE/lib/libupnp.so

    cp -p $LIBS_ROOT/libixml.so.2.0.8 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libixml.so.2.0.8 $DEPLOY_SYSROOT_FIRMWARE/lib/libixml.so.2
    ln -sf libixml.so.2.0.8 $DEPLOY_SYSROOT_FIRMWARE/lib/libixml.so

    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libgcc_s.so $DEPLOY_SYSROOT_FIRMWARE/lib/
    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libgcc_s.so.1 $DEPLOY_SYSROOT_FIRMWARE/lib/

    cp -p $LIBS_ROOT/libnetlink.so $DEPLOY_SYSROOT_FIRMWARE/lib/
}

function build_plugin_packages() {
    pushd $PLUGINS_ROOT
    make
    popd
}

function unpack_and_copy_plugins() {
    pushd $PLUGINS_PACKAGES
    mkdir temp
    for package in `find ./*.ipk -type f`
    do
        tar -zxf $package -C ./temp
        pushd temp
        tar -zxf ./data.tar.gz
        mv ./opt/firmware/plugins/* $DEPLOY_SYSROOT_FIRMWARE/plugins
        popd
    done
    rm -rf temp
    popd
}

function sysroot_copy_plugin_files() {
    local PYTHON="$(which python3)"
    if [ -z "$PYTHON" ];
    then
        echo " ! ! !"
        echo "Plugins NOT installed: python3 missing"
        echo " ! ! !"
        return 0
    fi

    echo "Copying plugin files..."
    pushd $PLUGINS_GATEWAYS_ROOT
    python3 dev_utils/device_generation.py
    popd
    build_plugin_packages
    unpack_and_copy_plugins
}

function download_custom_packages() {
    pushd $CUSTOM_PACKAGES_ROOT
    for packageURL in $CUSTOM_PACKAGES; do
        package=$(basename $packageURL)

        if [ ! -f $package ]
        then
            echo "Downloading custom package: $packageURL"
            wget --tries=10 --timeout=30 --progress=dot:mega $packageURL || rm -f $package
        fi
    done
    popd
}

function unpack_and_copy_custom_packages() {
    pushd $CUSTOM_PACKAGES_ROOT
    mkdir temp

    for packageURL in $CUSTOM_PACKAGES; do
        package=$(basename $packageURL)
        if [ -f $package ]
        then
            tar -zxf $package -C ./temp
            tar -zxf ./temp/data.tar.gz -C $DEPLOY_SYSROOT
        fi
    done
    rm -rf temp
    popd
}

function sysroot_copy_custom_packages_files() {
    if [ -z "$CUSTOM_PACKAGES" ]
    then
        return 0
    fi

    echo "Copying custom packages files..."

    mkdir -p $CUSTOM_PACKAGES_ROOT
    download_custom_packages
    unpack_and_copy_custom_packages
}

function deploy_a20() {
    echo "Stopping ha daemons...$SOURCES_ROOT"

# try stop for old architecture
    eval_cmd "[ -e /opt/ezlo/bin/stop.sh ] && [ -x /opt/ezlo/bin/stop.sh ] && /opt/ezlo/bin/stop.sh"


# try stop for recent architecture
    eval_cmd "[ -e $DEPLOY_DIR/bin/stop.sh ] && [ -x $DEPLOY_DIR/bin/stop.sh ] && $DEPLOY_DIR/bin/stop.sh"

    sysroot_make_folders

    mkdir -p $DEPLOY_SYSROOT/root/.ssh/ssh_keys/
    mkdir -p $DEPLOY_SYSROOT/lib/
    mkdir -p $DEPLOY_SYSROOT/etc/rc2.d/
    mkdir -p $DEPLOY_SYSROOT/etc/systemd/system/
    mkdir -p $DEPLOY_SYSROOT/etc/logrotate.d
    mkdir -p $DEPLOY_SYSROOT/etc/cron.daily
    mkdir -p $DEPLOY_SYSROOT/etc/cron.hourly

    mkdir -p $DEPLOY_SYSROOT/var/log/updates/
    mkdir -p $DEPLOY_SYSROOT/run/spread/
    ln -sf /run $DEPLOY_SYSROOT/var/run

    sysroot_copy_firmware
    sysroot_copy_toolchain_files
    sysroot_copy_plugin_files

    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libstdc++.so.6.0.24 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libstdc++.so.6.0.24 $DEPLOY_SYSROOT_FIRMWARE/lib/libstdc++.so.6
    ln -sf libstdc++.so.6.0.24 $DEPLOY_SYSROOT_FIRMWARE/lib/libstdc++.so

    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libatomic.so.1.2.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libatomic.so.1.2.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libatomic.so
    ln -sf libatomic.so.1.2.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libatomic.so.1


    cp -prf $SYSTEM_ROOT/a20.1/etc $DEPLOY_SYSROOT
    cp -prf $SYSTEM_ROOT/a20.1/root $DEPLOY_SYSROOT

    ln -sf /opt/firmware/scripts/firmware_startup.sh $DEPLOY_SYSROOT/etc/rc2.d/S99start.firmware

    ln -sf /lib/arm-linux-gnueabihf/ld-2.19.so $DEPLOY_SYSROOT/lib/ld-linux.so.3

    cp -p $LIBS_ROOT/libliveMedia.so.64.0.3 $DEPLOY_SYSROOT_FIRMWARE/lib
    ln -sf libliveMedia.so.64.0.3 $DEPLOY_SYSROOT_FIRMWARE/lib/libliveMedia.so.64
    ln -sf libliveMedia.so.64.0.3 $DEPLOY_SYSROOT_FIRMWARE/lib/libliveMedia.so

    cp -p $LIBS_ROOT/libBasicUsageEnvironment.so.1.0.1 $DEPLOY_SYSROOT_FIRMWARE/lib
    ln -sf libBasicUsageEnvironment.so.1.0.1 $DEPLOY_SYSROOT_FIRMWARE/lib/libBasicUsageEnvironment.so.1
    ln -sf libBasicUsageEnvironment.so.1.0.1 $DEPLOY_SYSROOT_FIRMWARE/lib/libBasicUsageEnvironment.so

    cp -p $LIBS_ROOT/libUsageEnvironment.so.3.1.0 $DEPLOY_SYSROOT_FIRMWARE/lib
    ln -sf libUsageEnvironment.so.3.1.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libUsageEnvironment.so.3
    ln -sf libUsageEnvironment.so.3.1.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libUsageEnvironment.so

    cp -p $LIBS_ROOT/libgroupsock.so.8.2.2 $DEPLOY_SYSROOT_FIRMWARE/lib
    ln -sf libgroupsock.so.8.2.2 $DEPLOY_SYSROOT_FIRMWARE/lib/libgroupsock.so.8
    ln -sf libgroupsock.so.8.2.2 $DEPLOY_SYSROOT_FIRMWARE/lib/libgroupsock.so

    sysroot_perform_permissions
    sysroot_perform_deploy

    eval_cmd "$DEPLOY_DIR/bin/start.sh &"
}

function deploy_h2() {
    echo "Stopping ha daemons...$SOURCES_ROOT"

    eval_cmd "if [[ -x /etc/init.d/monit ]]; then /etc/init.d/monit stop; fi"
    eval_cmd "/etc/init.d/firmware stop"
    eval_cmd "opkg list-installed | grep rsync || opkg install http://archive.openwrt.org/releases/packages-18.06/arm_cortex-a7_neon-vfpv4/packages/rsync_3.1.3-1_arm_cortex-a7_neon-vfpv4.ipk"

    sysroot_make_folders
    sysroot_copy_firmware
    cp -p $LIBS_ROOT/libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/libwebsockets.so
    sysroot_copy_plugin_files
    sysroot_copy_custom_packages_files

    rm -rf $DEPLOY_SYSROOT_FIRMWARE/plugins/siren

    sysroot_perform_permissions
    sysroot_perform_deploy

    eval_cmd "sync"
    eval_cmd "if [[ -e /usr/lib/libwebsockets.so.14 ]]; then rm /opt/firmware/lib/libwebsockets.* ; fi"
    eval_cmd "/etc/init.d/firmware start"
}

function deploy_h2_secure() {
    echo "Stopping ha daemons...$SOURCES_ROOT"

    eval_cmd "if [[ -x /etc/init.d/monit ]]; then /etc/init.d/monit stop; fi"
    eval_cmd "/etc/init.d/firmware stop"
    eval_cmd "/etc/init.d/failsafe_reboot stop"
    eval_cmd "opkg list-installed | grep rsync || opkg install http://archive.openwrt.org/releases/packages-18.06/arm_cortex-a7_neon-vfpv4/packages/rsync_3.1.3-1_arm_cortex-a7_neon-vfpv4.ipk"

    sysroot_make_folders
    sysroot_copy_firmware
    cp -p $LIBS_ROOT/libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/libwebsockets.so
    sysroot_copy_plugin_files
    sysroot_copy_custom_packages_files

    sysroot_perform_permissions
    sysroot_perform_deploy

    eval_cmd "sync"
    eval_cmd "if [[ -e /usr/lib/libwebsockets.so.14 ]]; then rm /opt/firmware/lib/libwebsockets.* ; fi"
    eval_cmd "/etc/init.d/failsafe_reboot start"
    eval_cmd "/etc/init.d/firmware start"
}

function deploy_g150() {
    echo "Stopping ha daemons...$SOURCES_ROOT"

    eval_cmd "/etc/init.d/firmware stop"
    eval_cmd "opkg list-installed | grep rsync || opkg install http://archive.openwrt.org/barrier_breaker/14.07/ramips/mt7620n/packages/packages/rsync_3.1.1-2_ramips_24kec.ipk"

    sysroot_make_folders
    sysroot_copy_firmware
    cp -p $LIBS_ROOT/libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libwebsockets.so.14 $DEPLOY_SYSROOT_FIRMWARE/lib/libwebsockets.so
    sysroot_copy_plugin_files

    rm -rf $DEPLOY_SYSROOT_FIRMWARE/plugins/siren

    sysroot_perform_permissions
    sysroot_perform_deploy

    eval_cmd "sync"
    eval_cmd "if [[ -e /usr/lib/libwebsockets.so.14 ]]; then rm /opt/firmware/lib/libwebsockets.* ; fi"
    eval_cmd "/etc/init.d/firmware start"
}

function deploy_local() {
    echo "Stopping ha daemons...$SOURCES_ROOT"

    # try stop for existing architecture - run in target destination
    eval_cmd "[ -x $DEPLOY_DIR/bin/stop.sh ] && $DEPLOY_DIR/bin/stop.sh"

    sysroot_wipe_deploy_destination
    sysroot_make_folders

    eval_cmd "mkdir -p $LOG_DIR"

    sysroot_copy_firmware
    sysroot_copy_toolchain_files
    sysroot_copy_plugin_files

    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libstdc++.so.6.0.19 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libstdc++.so.6.0.19 $DEPLOY_SYSROOT_FIRMWARE/lib/libstdc++.so.6
    ln -sf libstdc++.so.6.0.19 $DEPLOY_SYSROOT_FIRMWARE/lib/libstdc++.so

    cp -p $TOOLCHAIN_SYSTEM_PATH/lib/libatomic.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/
    ln -sf libatomic.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libatomic.so
    ln -sf libatomic.so.1.0.0 $DEPLOY_SYSROOT_FIRMWARE/lib/libatomic.so.1

    sysroot_perform_permissions
    sysroot_perform_deploy

#   Do NOT start daemons - run'em  manually, start only spread
#    eval_cmd "$DEPLOY_DIR/bin/start.sh -l $LOG_DIR > /dev/null 2>&1 &"
    $DEPLOY_DIR/bin/spread.sh -l $LOG_DIR &
    $DEPLOY_DIR/bin/mosquitto.sh -l $LOG_DIR &
}

sleep 1
case "$HUB_TYPE" in
    "a20.1" )
        deploy_a20
        ;;

    "h2.1" )
	deploy_h2
	;;

    "h2_secure.1" )
        deploy_h2_secure
        ;;

    "g150" )
        deploy_g150
        ;;

    "local" )
        deploy_local
        ;;

    *)
        echo "Invalid hub type detected: $HUB_TYPE"
        exit 6
        ;;
esac

rm -rf "$DEPLOY_SYSROOT"

echo "$(date +'%F %T') Done"
