#!/bin/bash

# create aosp sparse directory of modified and newly created files

# android aosp location

AOSP_ROOTDIR=~/aospDevelopment/aosp


mkdir -p ./aosp/device/lge/mako/bin/testCanonicalCharDrv
mkdir -p ./aosp/hardware/libhardware/include/hardware
mkdir -p ./aosp/hardware/libhardware/modules/canonical_char_drv

# modified files
cp $AOSP_ROOTDIR/device/lge/mako/device.mk ./aosp/device/lge/mako/device.mk
cp $AOSP_ROOTDIR/device/lge/mako/init.mako.rc ./aosp/device/lge/mako/init.mako.rc
cp $AOSP_ROOTDIR/hardware/libhardware/modules/Android.mk ./aosp/hardware/libhardware/modules/Android.mk
cp $AOSP_ROOTDIR/hardware/libhardware/include/hardware/canonical_char_drv.h ./aosp/hardware/libhardware/include/hardware/canonical_char_drv.h

# new files
cp $AOSP_ROOTDIR/device/lge/mako/bin/Android.mk ./aosp/device/lge/mako/bin/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/bin/testCanonicalCharDrv/Android.mk ./aosp/device/lge/mako/bin/testCanonicalCharDrv/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/bin/testCanonicalCharDrv/test_canonical_char_drv.c ./aosp/device/lge/mako/bin/testCanonicalCharDrv/test_canonical_char_drv.c
cp $AOSP_ROOTDIR/hardware/libhardware/modules/canonical_char_drv/Android.mk ./aosp/hardware/libhardware/modules/canonical_char_drv/Android.mk
cp $AOSP_ROOTDIR/hardware/libhardware/modules/canonical_char_drv/libcanonical_char_drv.c ./aosp/hardware/libhardware/modules/canonical_char_drv/libcanonical_char_drv.c


