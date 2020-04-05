#!/bin/bash

readonly cwd=$(pwd)
GENERATOR='-G "Unix Makefiles"'
BUILD_CONFIG=Release

echo $cwd
mkdir -p ./build
cd ./build
pwd

#Build our cmake comand line
cmake_cmd="cmake .. $GENERATOR
-DCMAKE_BUILD_TYPE=$BUILD_CONFIG
-DLIB_TYPE=STATIC 
-DCMAKE_CXX_COMPILER=/usr/bin/g++ 
-DCMAKE_C_COMPILER=/usr/bin/gcc
-Wno-dev"

echo executing cmake using: $cmake_cmd

# execute cmake
eval $cmake_cmd

# Findout how many CPU's we have
#CPUS ?= $(shell sysctl -n hw.ncpu || echo 1)
if [[ "$OSTYPE" == "darwin"* ]]; then
	NPROC=`sysctl -n hw.ncpu`
else
	NPROC=`nproc`
fi

# Run parrallell make
make -j $NPROC --quiet
#make install