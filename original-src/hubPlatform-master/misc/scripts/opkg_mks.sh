#!/bin/bash

#a target toolchain map
# TODO: cmake.toolchain files should be rename too

dir_name=`dirname $0`
root_dir=`realpath ${dir_name}`/../..

source "${root_dir}/included_hubs"
declare -A toolchains
declare -A hubs
[ ${INCLUDE_h2_1_h2_secure_1}y != "no"y ] && toolchains+=( ["arm-musl-7.4.0_minsizerel"]="arm-openwrt.toolchain" ) && hubs+=( ["arm-musl-7.4.0_minsizerel"]="h2.1 h2_secure.1" )
[ ${INCLUDE_g150}y != "no"y ] && toolchains+=( ["mips-uclibc-4.8-linaro_minsizerel"]="mips.toolchain" ) && hubs+=( ["mips-uclibc-4.8-linaro_minsizerel"]="g150" )
[ ${INCLUDE_beewave_1}y != "no"y ] && toolchains+=( ["mips-musl-7.4.0_minsizerel"]="mips-musl.toolchain" ) && hubs+=( ["mips-musl-7.4.0_minsizerel"]="beewave.1" )

echo "Included toolchains: h2.1/h2_secure.1: ${INCLUDE_h2_1_h2_secure_1}"
echo "Included toolchains: g150: ${INCLUDE_g150}"
echo "Included toolchains: beewave.1: ${INCLUDE_beewave_1}"

supported_cmds=( "update" "check" "pack" )

process_cml_args()
{
POSITIONAL=()

while [[ $# -gt 0 ]]; do
key="$1"

case $key in
    --no-clean)
    no_clean="yes"
    shift;
    ;;

    --no-build)
    no_build="yes"
    shift;
    ;;

    --no-install)
    no_install="yes"
    shift;
    ;;
    
    --make-option)
    make_option="$2"
    shift;
    shift;
    ;;

    update)
    cmd="update"
    shift;
    ;;
    
    check)
    cmd="check"
    shift;
    ;;

    pack)
    cmd="pack"
    shift;
    ;;

    --pkg-version-build)
    pkg_version_build="--build_number $2"
    shift;
    shift;
    ;;

    --pkg-version-env)
    pkg_version_env="--env $2"
    shift;
    shift;
    ;;

    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac

done

set -- "${POSITIONAL[@]}" # restore positional parameters
}

check_cml_args()
{
if [ $# -lt 1 ]; then
    echo "Insufficient ammount of arguments."
    echo "Usage:"
    echo " ./opkg_package_mks.sh [--no-build | --no-install | --make-option value | --pkg-version-build value | --pkg-version-env value] update | check | pack"
    
    exit 1
fi

local proper_cmd=false

for i in ${supported_cmds[@]}; do
    if [ ${cmd}y = ${i}y ]; then proper_cmd=true; fi
done

if [ "$proper_cmd" = false ]; then
    echo "Unsupported command, use the next ones:" ${supported_cmds[@]}
    exit 1
fi

# for local mode (check mks before make a PR), use all cores, Jenkins will specify amount of cores explicitly
echo "make_option passed from outside to: ${make_option}"

make_option=${make_option:-"-j$(nproc)"}
echo "make_option set to: ${make_option}"
}

hub_cmake_build()
{
    local toolchain=$1
    local hub=$2

    mkdir -p ${root_dir}/build/${toolchain}/${hub}
    (cd ${root_dir}/build/${toolchain}/${hub} && cmake -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_TOOLCHAIN_FILE=misc/cmake/${toolchains[$toolchain]}.cmake -DHUB_TYPE=${hub} ../../..)
}
#---------------------------------------------------------------------------------------

process_cml_args $@
check_cml_args $@

if [ ${no_build}y != "yes"y ]; then
    for toolchain in ${!toolchains[@]}; do
        mkdir -p ${root_dir}/build/${toolchain}

        for hub in ${hubs[$toolchain]}; do
            [ ! -d ${root_dir}/build/${toolchain}/${hub} ] && hub_cmake_build ${toolchain} ${hub}
            [ ${no_clean}y != "yes"y ] && (cd ${root_dir}/build/${toolchain}/${hub} && make clean)

            (cd ${root_dir}/build/${toolchain}/${hub} && make ${make_option})
        done
    done
fi

if [ ${no_install}y != "yes"y ]; then
    for toolchain in ${!toolchains[@]}; do
        for hub in ${hubs[$toolchain]}; do
            (cd ${root_dir}/build/${toolchain}/${hub} && rm -rf ./installed/* && make install)
        done
    done
fi

for toolchain in ${!toolchains[@]}; do
    toolchain_dirs+=${root_dir}/build/${toolchain},
done

# remove a trailing ',' symbol
toolchain_dirs=${toolchain_dirs::${#toolchain_dirs}-1}

${root_dir}/misc/scripts/openwrt/pkgdeps.py --toolchain_dirs ${toolchain_dirs} --opkg_dir ${root_dir}/misc/opkg --action ${cmd} ${pkg_version_build} ${pkg_version_env}
