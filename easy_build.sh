#!/bin/bash

export MY_INSTALL_DIR=$HOME/.local
mkdir -p cmake/build
cd cmake/build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR  ../..
make -j ${nproc}