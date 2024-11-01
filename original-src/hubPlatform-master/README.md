General
==============

[Code Style](https://ezloproject.atlassian.net/wiki/spaces/PM/pages/217940280/Code+style+recommendations)

[Git Workflow](https://confluence.mios.com/display/ELFD/Git+Workflow)

[HubPlatform Build Manual](https://confluence.mios.com/display/ELFD/HubPlatform+Build+Manual)

Get Sources
==============

    git clone git@github.com:ezloteam/hubPlatform.git --recurse-submodules

If you didn't have submodules in your source folder before, need to execute:

    git submodule init

After each `git pull` and `git update` commands, need to sync submodules by commands:

    git submodule sync --recursive
    git submodule update --recursive

Build instructions(CMake)
==============

Recommended structure for build folders:

    <SOURCES>/build/arm_debug
    <SOURCES>/build/amd64_release

hubPlatform code can be compiled only with one of toolchains. Use `CMAKE_TOOLCHAIN_FILE` cmake flag for generating particular configuration.

**Important:** If you want to use different toolchain for existing cmake directory, need to remove all content of this folder before generating.

| Architecture    | Toolchain File                           |
| --------------- | ---------------------------------------- |
| **arm**         | `misc/cmake/arm.toolchain.cmake`         |
| **amd64**       | `misc/cmake/amd64.toolchain.cmake`       |
| **arm-openwrt** | `misc/cmake/arm-openwrt.toolchain.cmake` |
| **mips**        | `misc/cmake/mips.toolchain.cmake`        |

Go to sources folder and create folder for building, example for `amd64` debug configuration:

    mkdir build/amd64_debug
    cd build/amd64_debug
    cmake -DCMAKE_TOOLCHAIN_FILE=misc/cmake/amd64.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ../..

For disabling colored cmake output you can add `HUB_CMAKE_COLORED_OUTPUT=OFF` flag:

    cmake -DCMAKE_TOOLCHAIN_FILE=misc/cmake/amd64.toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DHUB_CMAKE_COLORED_OUTPUT=OFF ../..

Other flags:

| Flag                              | Value Type | Default Value | Description                                                                                |
|-----------------------------------|----------- |---------------|--------------------------------------------------------------------------------------------|
| **`WITH_TESTS`**                  | BOOL       | `ON`          | Need to build tests                                                                        |
| **`IS_NEED_COVERAGE`**            | BOOL       | `OFF`         | Need to create target for tests coverage                                                   |
| **`HUB_CMAKE_COLORED_OUTPUT`**    | BOOL       | `ON`          | Use colored output for configuration messages                                              |
| **`HUB_FIRMWARE_COLORED_OUTPUT`** | BOOL       | `OFF`         | Use colored output for firmware output                                                     |
| **`HUB_DEPLOY_BLE`**              | BOOL       | `OFF`         | Deploy Bluetooth addon                                                                     |
| **`HUB_DEPLOY_ZIGBEE`**           | BOOL       | `OFF`         | Deploy ZigBee addon                                                                        |
| **`HUB_WITH_LTE`**                | BOOL       | `OFF`         | Enable LTE support for hubs with LTE chip                                                  |
| **`HUB_ARM_DEPLOY_IP`**           | STRING     | `127.0.0.1`   | Ezlo Hub IP for deploying firmware                                                         |
| **`HUB_ARM_DEPLOY_PORT`**         | STRING     | `22`          | Ezlo Hub port for deploying firmware                                                       |
| **`HUB_MAC`**                     | STRING     |               | Mac address of hub which we need to use                                                    |
| **`HUB_SERIAL`**                  | STRING     | `-serial-`    | Serial/PK device of hub which we need to use                                               |
| **`FIRMWARE_BUILD_NUMBER`**       | STRING     | `0`           | Build number to include to firmware and addons                                             |
| **`HUB_FIRMWARE_LOGS_LEVEL`**     | STRING     | `DEBUG`       | Firmware log level to use by default. One of: `ERROR`, `WARNING`, `INFO`, `DEBUG`, `TRACE` |
| **`LOGGER_LOG_THREAD_ID`**        | BOOL       | `OFF`         | Print a tid (thread id) for each log message line

Make Pre-requests
==============

Before the build step STAGING_DIR environment should be set. You can set this variable before make comamnd

    STAGING_DIR=${STAGING_DIR:-/missing} make -j4

Or you can export it manually

    export STAGING_DIR=${STAGING_DIR:-/missing}
    make -j4

Or you can define it in /etc/profile or any other shell profile file

    STAGING_DIR=${STAGING_DIR:-/missing}
    # After re-sourcing the profile file or rebooting the system STAGING_DIR warning should be gone

Run unit tests
==============

For running tests could be used `ctest` tool. `ctest` command need to execute in the build folder. Examples:

| Command                     | Description                                                                  |
| ----------------------------|----------------------------------------------------------------------------- |
| `ctest`                     | Run all existing tests                                                       |
| `ctest -j2`                 | Run all existing tests in 2 threads                                          |
| `ctest --output-on-failure` | Show test output if it's failed                                              |
| `ctest -R lua`              | Run tests modules which has `lua` word in the name                           |
| `ctest -R lua -VV`          | Run tests modules which has `lua` word in the name and show all tests output |

Run Code Style Formatter
==============

You could build pretty target for auto-formatting code. But You need to install clang-format 6 for correct working of it.
Example for Ubuntu:

    sudo apt-get install clang-format-6.0
    sudo ln -s /usr/bin/clang-format-6.0 /usr/bin/clang-format

Move to <SOURCES>/build/amd64_debug folder and run:

    make pretty

Code should be changed according current code style.

Deploy
==============

ARM binaries could be deployed on A20 and H2 hubs by `deploy` target. Add `cmake` flags with your hub ip and type:

    cmake -DCMAKE_TOOLCHAIN_FILE=misc/cmake/arm.toolchain.cmake -DHUB_ARM_DEPLOY_IP=192.168.11.1 -DHUB_TYPE=h2.1 ../..

Amd64 binaries could be deployed on PC by `deploy` target. Log files will be saved in _$HOME/.hubPlatform/log_ directory.

Add `cmake` flag with hub type `local`:

    cmake -DCMAKE_TOOLCHAIN_FILE=misc/cmake/amd64.toolchain.cmake -DHUB_TYPE=local ../..

After building you can run `deploy` target:

    make deploy
