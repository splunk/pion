#!/bin/sh
echo Switching to gcc-$1
ln -sf /usr/bin/cc-$1 /usr/bin/cc
ln -sf /usr/bin/gcc-$1 /usr/bin/gcc
ln -sf /usr/bin/cpp-$1 /usr/bin/cpp
ln -sf /usr/bin/c++-$1 /usr/bin/c++
ln -sf /usr/bin/g++-$1 /usr/bin/g++
ln -sf /usr/bin/gcov-$1 /usr/bin/gcov
