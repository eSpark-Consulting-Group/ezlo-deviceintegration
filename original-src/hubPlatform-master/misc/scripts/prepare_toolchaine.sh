#!/bin/bash

#
# move values of command line arguments into variables
#
argc=$#
argv=($@)
for (( j=0; j<argc; j++ )); do
	#echo ${argv[j]}
	PARAM=`echo ${argv[j]} | awk -F= '{print $1}'`
	VALUE=`echo ${argv[j]} | awk -F= '{print $2}'`
	#echo ${PARAM}
	#echo ${VALUE}
	export ${PARAM}=${VALUE}
done

if [ -z "${HUB_CMAKE_COLORED_OUTPUT}" ] || [ "${HUB_CMAKE_COLORED_OUTPUT}" == "OFF" ] || [ "${HUB_CMAKE_COLORED_OUTPUT}" == "0" ];
then
    COLOR_RESET='' # No Color
    COLOR_BLACK=''
    COLOR_DARK_GRAY=''
    COLOR_BLUE=''
    COLOR_LIGHT_BLUE=''
    COLOR_GREEN=''
    COLOR_LIGHT_GREEN=''
    COLOR_CYAN=''
    COLOR_LIGHT_CYAN=''
    COLOR_RED=''
    COLOR_LIGHT_RED=''
    COLOR_MAGENTA=''
    COLOR_LIGHT_MAGENTA=''
    COLOR_BROWN=''
    COLOR_YELLOW=''
    COLOR_LIGHT_GRAY=''
    COLOR_WHITE=''
else
    COLOR_RESET='\033[0m' # No Color
    COLOR_BLACK='\033[0;30m'
    COLOR_DARK_GRAY='\033[1;30m'
    COLOR_BLUE='\033[0;34m'
    COLOR_LIGHT_BLUE='\033[1;34m'
    COLOR_GREEN='\033[0;32m'
    COLOR_LIGHT_GREEN='\033[1;32m'
    COLOR_CYAN='\033[0;36m'
    COLOR_LIGHT_CYAN='\033[1;36m'
    COLOR_RED='\033[0;31m'
    COLOR_LIGHT_RED='\033[1;31m'
    COLOR_MAGENTA='\033[0;35m'
    COLOR_LIGHT_MAGENTA='\033[1;35m'
    COLOR_BROWN='\033[0;33m'
    COLOR_YELLOW='\033[1;33m'
    COLOR_LIGHT_GRAY='\033[0;37m'
    COLOR_WHITE='\033[1;37m'
fi

# colors/variables
COLOR_ERROR="${COLOR_RED}"
COLOR_WARN="${COLOR_YELLOW}"
COLOR_ATTENTION="${COLOR_LIGHT_CYAN}"
COLOR_SUCCESS="${COLOR_GREEN}"
COLOR_INFO="${COLOR_BLUE}"
COLOR_NOTICE="${COLOR_MAGENTA}"
COLOR_DOTS="${COLOR_DARK_GRAY}"
COLOR_SEPARATOR="${COLOR_LIGHT_MAGENTA}"
COLOR_VAR_NAME="${COLOR_BLUE}"
COLOR_VAR_DESC="${COLOR_BROWN}"
COLOR_VAR_VAL="${COLOR_LIGHT_GRAY}"
COLOR_PROJECT_NAME="${COLOR_LIGHT_BLUE}"

simple_find_tool_program () { # program_name, var_name_to_export_full_path, is_optional("yes" or "no")
	#echo $1
	#echo $2
	TMP_CMD="export $2=`which $1`"
	$TMP_CMD
	TMP_CMD="echo ${!2}"
	#echo "TMP_CMD is" $TMP_CMD
	TMP_VAL=`$TMP_CMD`
	#echo "TMP_VAL is" $TMP_VAL
	if [ "$TMP_VAL" = "" ];
	then
		TMP_CMD="export $2=/usr/local/bin/$1"
		$TMP_CMD
		TMP_CMD="echo ${!2}"
		TMP_VAL=`$TMP_CMD`
		if [ -f "$TMP_VAL" ];
		then
			#echo -e "${COLOR_SUCCESS}SUCCESS: $2 found as $TMP_VAL" "${COLOR_RESET}"
			return 0
		fi
		#TMP_CMD="export $2=/opt/local/bin/$1"
		#$TMP_CMD
		#TMP_CMD="echo ${!2}"
		#TMP_VAL=`$TMP_CMD`
		#if [ -f "$TMP_VAL" ];
		#then
		#	#echo -e "${COLOR_SUCCESS}SUCCESS: $2 found as $TMP_VAL" "${COLOR_RESET}"
		#	return 0
		#fi
	fi
	if [ -f "$TMP_VAL" ];
	then
		#echo -e "${COLOR_SUCCESS}SUCCESS: $2 found as $TMP_VAL" "${COLOR_RESET}"
		return 0
	fi

	echo -e "${COLOR_ERROR}error: $2 tool was not found by deps build script${COLOR_RESET}"
	cd $WORKING_DIR_OLD
	exit -1
}

echo -e "${COLOR_SEPARATOR}==================== ${COLOR_PROJECT_NAME}PREPARE TOOLCHAINE${COLOR_SEPARATOR} ================================${COLOR_RESET}"

simple_find_tool_program "wget" "WGET"
simple_find_tool_program "tar" "TAR"

if [ -z "${ARCH}" ];
then
	# if we don't have explicit ARCH=something from command line arguments
	ARCH="amd64"
elif [ "$ARCH" != "arm" ] && [ "$ARCH" != "amd64" ] && [ "$ARCH" != "arm-openwrt" ] && [ "$ARCH" != "mips" ] && [ "$ARCH" != "mips-musl" ];
then
	echo -e "${COLOR_ERROR}error: ${COLOR_VAR_VAL}'$ARCH'${COLOR_ERROR} architecture is not supported.${COLOR_RESET}"
	echo -e "${COLOR_ERROR}error: There are supported only: ${COLOR_VAR_VAL}'arm'${COLOR_ERROR}, ${COLOR_VAR_VAL}'amd64'${COLOR_ERROR}, ${COLOR_VAR_VAL}'arm-openwrt'${COLOR_ERROR}, ${COLOR_VAR_VAL}'mips'${COLOR_ERROR}, ${COLOR_VAR_VAL}'mips-musl'${COLOR_ERROR} architectures.${COLOR_RESET}"
	exit -1
fi

export TOOLCHAINS_PATH=/usr/local/toolchains
export TOOLCHAINS_DOWNLOADED_PATH=$TOOLCHAINS_PATH/downloads
export EXTRA_TAR_OPTIONS=""

if [ "$ARCH" = "arm" ];
then
	export TOOLCHAIN_NAME=gcc7.2.0-arm-v1.7
elif [ "$ARCH" = "amd64" ];
then
	export TOOLCHAIN_NAME=gcc4.8.3-amd64-v1.6
elif [ "$ARCH" = "arm-openwrt" ];
then
	export TOOLCHAIN_NAME=gcc7.4.0_musl_eabi-arm-v1.7
	export EXTRA_TAR_OPTIONS="${TOOLCHAIN_NAME}/gcc-7.4.0_musl_eabi-arm --strip-components=2"
elif [ "$ARCH" = "mips" ];
then
	export TOOLCHAIN_NAME=gcc4.8-linaro-mipsel-v1.9
	export EXTRA_TAR_OPTIONS="${TOOLCHAIN_NAME}/gcc-4.8-linaro-mipsel --strip-components=2"
elif [ "$ARCH" = "mips-musl" ];
then
	export TOOLCHAIN_NAME=gcc7.4.0_musl-mipsel-v1.1
	export EXTRA_TAR_OPTIONS="${TOOLCHAIN_NAME}/gcc-7.4.0_musl-mipsel --strip-components=2"
fi

export TOOLCHAIN_ARCH_NAME=${TOOLCHAIN_NAME}.tar.bz2
export TOOLCHAIN_PATH=${TOOLCHAINS_PATH}/${TOOLCHAIN_NAME}
export TOOLCHAINS_STORAGE_SERVER="https://storage.googleapis.com/ezlo-toolchains"

echo -e "${COLOR_VAR_NAME}ARCH${COLOR_DOTS}...................${COLOR_VAR_DESC}Building for architecture${COLOR_DOTS}..............${COLOR_VAR_VAL}$ARCH${COLOR_RESET}"
echo -e "${COLOR_VAR_NAME}WGET${COLOR_DOTS}..........................................................${COLOR_VAR_VAL}$WGET${COLOR_RESET}"
echo -e "${COLOR_VAR_NAME}TAR${COLOR_DOTS}...........................................................${COLOR_VAR_VAL}$TAR${COLOR_RESET}"
echo -e "${COLOR_VAR_NAME}TOOLCHAIN_NAME${COLOR_DOTS}................................................${COLOR_VAR_VAL}$TOOLCHAIN_NAME${COLOR_RESET}"
echo -e "${COLOR_VAR_NAME}TOOLCHAIN_ARCH_NAME${COLOR_DOTS}...........................................${COLOR_VAR_VAL}$TOOLCHAIN_ARCH_NAME${COLOR_RESET}"
echo -e "${COLOR_VAR_NAME}TOOLCHAIN_INTERNAL_LINK${COLOR_DOTS}.......................................${COLOR_VAR_VAL}$TOOLCHAINS_STORAGE_SERVER/$TOOLCHAIN_ARCH_NAME${COLOR_RESET}"

if [ ! -d "$TOOLCHAIN_PATH" ];
then
    mkdir -p $TOOLCHAINS_PATH
    if [ ! -d $TOOLCHAINS_PATH ];
    then
        echo " "
        echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
        echo -e "${COLOR_ERROR}error: ${COLOR_VAR_VAL}${TOOLCHAINS_PATH}${COLOR_ERROR} folder not created!${COLOR_RESET}"
        echo -e "${COLOR_ERROR}Create ${COLOR_VAR_VAL}${TOOLCHAINS_PATH}${COLOR_ERROR} folder and give permissions for writing here to current user.${COLOR_RESET}"
        echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
        cd $WORKING_DIR_OLD
        exit -1
    fi

    if [ ! -f $TOOLCHAINS_DOWNLOADED_PATH/$TOOLCHAIN_ARCH_NAME ];
    then
        echo -e "${COLOR_SEPARATOR}==================== ${COLOR_PROJECT_NAME}DOWNLOAD TOOLCHAINE ARCH${COLOR_SEPARATOR} ==========================${COLOR_RESET}"

        mkdir -p $TOOLCHAINS_DOWNLOADED_PATH
        cd $TOOLCHAINS_DOWNLOADED_PATH
        wget $TOOLCHAINS_STORAGE_SERVER/$TOOLCHAIN_ARCH_NAME

        if [ ! -f $TOOLCHAINS_DOWNLOADED_PATH/$TOOLCHAIN_ARCH_NAME ];
        then
            echo " "
            echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
            echo -e "${COLOR_ERROR}Cannot download toolchain archive: ${COLOR_VAR_VAL}$TOOLCHAINS_STORAGE_SERVER/$TOOLCHAIN_ARCH_NAME${COLOR_RESET}"
            echo -e "${COLOR_ERROR}Try download: ${COLOR_VAR_VAL}$TOOLCHAINS_STORAGE_SERVER/$TOOLCHAIN_ARCH_NAME${COLOR_RESET}"
            echo -e "${COLOR_ERROR}Copy ${COLOR_VAR_VAL}$TOOLCHAIN_ARCH_NAME${COLOR_ERROR} to ${COLOR_VAR_VAL}${TOOLCHAINS_DOWNLOADED_PATH}${COLOR_RESET}"
            echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
            cd $WORKING_DIR_OLD
            exit -1
        fi
    fi

    echo -e "${COLOR_SEPARATOR}==================== ${COLOR_PROJECT_NAME}UNPACK TOOLCHAINE ARCH${COLOR_SEPARATOR} ============================${COLOR_RESET}"

    mkdir -p $TOOLCHAIN_PATH
    cd $TOOLCHAIN_PATH
    echo "tar -xjvf $TOOLCHAINS_DOWNLOADED_PATH/$TOOLCHAIN_ARCH_NAME $EXTRA_TAR_OPTIONS"
    tar -xjvf $TOOLCHAINS_DOWNLOADED_PATH/$TOOLCHAIN_ARCH_NAME $EXTRA_TAR_OPTIONS

    if [ ! -f "$TOOLCHAIN_PATH/version" ];
    then
        echo " "
        echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
        echo -e "${COLOR_ERROR}Cannot unpack toolchain archive: ${COLOR_VAR_VAL}$TOOLCHAINS_DOWNLOADED_PATH${COLOR_ERROR}/${COLOR_VAR_VAL}$TOOLCHAIN_ARCH_NAME${COLOR_RESET}"
        echo -e "${COLOR_SEPARATOR}=================================================${COLOR_RESET}"
        cd $WORKING_DIR_OLD
        exit -1
    fi

    echo -e "${COLOR_SEPARATOR}============== ${COLOR_PROJECT_NAME}TOOLCHAINE UNPACKED${COLOR_SEPARATOR} ==============${COLOR_RESET}"
else
    echo -e "${COLOR_SEPARATOR}==================== ${COLOR_PROJECT_NAME}TOOLCHAINE IS READY${COLOR_SEPARATOR} ===============================${COLOR_RESET}"
fi
