FROM --platform=linux/amd64 ubuntu:18.04 as builder

ARG ARCH
ARG TARGET_PLATFORM
ARG CI_COMMIT_SHORT_SHA
ARG FIRMWARE_BUILD_NUMBER

RUN apt-get update \
    && apt-get install -y software-properties-common git wget python3 python3-pip bzip2 unzip nasm yasm flex gawk libdbus-1-dev libglib2.0-dev libncurses5-dev zlib1g-dev openssh-client clang-format-6.0 libc6-i386 chrpath\
    && pip3 install jsonref requests \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/clang-format-6.0 /usr/bin/clang-format \
    && ln -s /usr/bin/clang-format-diff-6.0 /usr/bin/clang-format-diff \
    && mkdir -p /usr/local/toolchains \
    && apt purge --auto-remove cmake \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - |  tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
    && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
    && apt update \
    && apt install cmake-data=3.21.3-0kitware1ubuntu18.04.1 cmake=3.21.3-0kitware1ubuntu18.04.1 -y
# provide an uid (-u) as your main user on the host to be
# able to deal with files generated within the container
# by the user created at this line;
# a primary group has to be created/specified, look at
# the documentation for the RUN command
RUN groupadd -f -g 1000 dev && \
    useradd -g 1000 -m -N -s /bin/bash -u 1000 dev

COPY CMakeLists.txt /src/
COPY deps/ /src/deps/
COPY firmware/ /src/firmware/
COPY system/ /src/system/
COPY misc/ /src/misc/

ENV PARALLEL_COUNT=$(nproc)

RUN mkdir /src/build
WORKDIR /src/build
RUN cmake \
    -DHUB_TYPE=vhub \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DHUB_FIRMWARE_COLORED_OUTPUT=OFF \
    -DHUB_CMAKE_COLORED_OUTPUT=OFF \
    -DCMAKE_TOOLCHAIN_FILE=misc/cmake/${ARCH}.toolchain.cmake \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DFIRMWARE_BUILD_NUMBER=${FIRMWARE_BUILD_NUMBER} \
    ..
 
RUN make -j$(nproc)
RUN make install

RUN mkdir /tmp/hub

RUN python3 ../misc/scripts/create_vhub_image.py --packages installed --plugins_dir ../deps/HubPlugins

RUN sed -i '2a if test ! -e /dev/stick;then exit;fi' /tmp/hub/hub_root/opt/firmware/bin/ha-zwaved.sh
RUN sed -i '2a if test ! -e /dev/zbstick;then exit;fi' /tmp/hub/hub_root/opt/firmware/bin/ha-zigbeed.sh
RUN sed -i '2a if test ! -d /var/run/dbus/;then exit;fi' /tmp/hub/hub_root/opt/firmware/bin/ha-bluetoothd.sh

RUN cp -vr /usr/local/toolchains/*/usr/lib/* /tmp/hub/hub_root/opt/firmware/lib/
RUN for f in `ls /tmp/hub/hub_root/opt/firmware/lib/*.so`;do chrpath -r /opt/firmware/lib/ $f >/dev/null;done
RUN if [ "${ARCH}" = "arm" ];then cp -v /usr/local/toolchains/gcc7.2.0-arm-v1.7/arm-linux-gnueabihf/lib/libstdc* /tmp/hub/hub_root/opt/firmware/lib/;fi
RUN cp -v /tmp/hub/hub_root/opt/firmware/lib/libjsonrpc.so /tmp/hub/hub_root/opt/firmware/lib/libjson-c.so

FROM --platform=$TARGET_PLATFORM gitlab-registry.nurd.com/ezlo/firmware/hubplatform/base:${CI_COMMIT_SHORT_SHA}

COPY --from=builder /usr/local/toolchains/*/usr/sbin/spread  /opt/firmware/sbin/spread

COPY --from=builder /tmp/hub/hub_root/ /

CMD ["/usr/sbin/start_services"]
