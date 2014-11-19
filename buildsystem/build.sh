#!/bin/bash
export ROOT_DIR=$(pwd)/..
export BUILDSYSTEM_DIR=$ROOT_DIR/buildsystem

mkdir -p $BUILDSYSTEM_DIR/builds/glibc
cd $BUILDSYSTEM_DIR/builds/glibc
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja $BUILDSYSTEM_DIR
ninja

#mkdir -p $BUILDSYSTEM_DIR/builds/musl
#cd $BUILDSYSTEM_DIR/builds/musl
#musl-gcc -shared -std=c11 -fPIC -o libKD.so $ROOT_DIR/sourcecode/kd.c -I$ROOT_DIR/distribution
