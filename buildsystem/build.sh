#!/bin/bash
export ROOT_DIR=$(pwd)/..
export BUILDSYSTEM_DIR=$ROOT_DIR/buildsystem

mkdir -p $BUILDSYSTEM_DIR/builds/glibc
cd $BUILDSYSTEM_DIR/builds/glibc
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja $BUILDSYSTEM_DIR
ninja

#mkdir -p $BUILDSYSTEM_DIR/builds/musl
#cd $BUILDSYSTEM_DIR/builds/musl
#cmake -DCMAKE_C_COMPILER=musl-gcc -DCMAKE_BUILD_TYPE=Debug -GNinja $BUILDSYSTEM_DIR
#ninja
