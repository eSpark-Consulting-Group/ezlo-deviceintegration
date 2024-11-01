#!/bin/bash

apt-get update && apt-get install clang-format clang-format-6.0 -y &&
rm /usr/bin/clang-format && rm /usr/bin/clang-format-diff &&
ln -s /usr/bin/clang-format-6.0 /usr/bin/clang-format &&
ln -s /usr/bin/clang-format-diff-6.0 /usr/bin/clang-format-diff
