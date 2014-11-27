#!/bin/bash
export ROOT_DIR=$(pwd)/..
export BUILDSYSTEM_DIR=$ROOT_DIR/buildsystem

mkdir -p $BUILDSYSTEM_DIR/builds/debug
cd $BUILDSYSTEM_DIR/builds/debug
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -GNinja $BUILDSYSTEM_DIR
ninja