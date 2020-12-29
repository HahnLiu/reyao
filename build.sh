#!/bin/sh

set -x 

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../reyao-build}
BUILD_TYPE=${BUILD_TYPE:-DEBUG}

mkdir -p $BUILD_DIR/$BUILD_TYPE \
    && cd $BUILD_DIR/$BUILD_TYPE \
    && cmake \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        $SOURCE_DIR \
    && make $* 

