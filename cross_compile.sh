#!/bin/sh

export KDIR=/usr/src/linux-stable
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64

make
