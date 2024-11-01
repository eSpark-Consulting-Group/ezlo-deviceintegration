FROM ubuntu:20.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -yq tzdata \
    && ln -fs /usr/share/zoneinfo/UTC /etc/localtime \
    && dpkg-reconfigure -f noninteractive tzdata \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update \
    && apt-get install -y software-properties-common git wget python3 python3-pip bzip2 unzip nasm yasm flex gawk libdbus-1-dev libglib2.0-dev libncurses5-dev zlib1g-dev openssh-client clang-format-6.0 libc6-i386 \
    && pip3 install jsonref requests public \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/clang-format-6.0 /usr/bin/clang-format \
    && ln -s /usr/bin/clang-format-diff-6.0 /usr/bin/clang-format-diff \
    && mkdir -p /usr/local/toolchains \
    && apt purge --auto-remove cmake \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - |  tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
    && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' \
    && apt update \
    && apt install cmake-data=3.21.3-0kitware1ubuntu20.04.1 cmake=3.21.3-0kitware1ubuntu20.04.1 -y

# provide an uid (-u) as your main user on the host to be
# able to deal with files generated within the container
# by the user created at this line;
# a primary group has to be created/specified, look at
# the documentation for the RUN command
RUN groupadd -f -g 1000 dev && \
    useradd -g 1000 -m -N -s /bin/bash -u 1000 dev
CMD ["/bin/bash"]
