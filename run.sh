#!/bin/sh
# build ptu with cmake

mkdir build
cd build
cmake ..
make
mv ptu ..
cd ..

