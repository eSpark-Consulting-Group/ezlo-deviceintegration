#example: 
# docker run -it -v ~/tmp:/tmp -w /home/dev plugin_package_checker ./check_plugin.sh /tmp/test_plugin.tgz /tmp/results.json

FROM ubuntu:18.04
RUN apt-get update \
    && apt-get install -y software-properties-common git wget python3 python3-pip bzip2 unzip nasm yasm flex gawk libdbus-1-dev libglib2.0-dev libncurses5-dev zlib1g-dev openssh-client clang-format-6.0 libc6-i386 \
    && pip3 install jsonref requests \
    && rm -rf /var/lib/apt/lists/* 
# provide an uid (-u) as your main user on the host to be
# able to deal with files generated within the container
# by the user created at this line;
# a primary group has to be created/specified, look at
# the documentation for the RUN command
RUN groupadd -f -g 1000 dev && \
    useradd -g 1000 -m -N -s /bin/bash -u 1000 dev

COPY check_plugin.sh plugin_package_checker /home/dev/
COPY HubPluginsUtils /home/dev/HubPluginsUtils

CMD ["/bin/bash"]
