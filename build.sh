#!/bin/sh

# build the varcreate shared object library first
mkdir -p libvarcreate/build
cd libvarcreate/build
cmake ..
make clean
make
sudo make install
cd ../..
sudo ldconfig

mkdir -p varcreate/build
cd varcreate/build
cmake ..
make clean
make
sudo make install
cd ../..
