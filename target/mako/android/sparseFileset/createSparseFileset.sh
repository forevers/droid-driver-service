#!/bin/bash

# create aosp sparse directory of modified and newly created files

# android aosp location

AOSP_ROOTDIR=~/aospDevelopment/aosp

mkdir -p ./aosp/device/lge/mako/bin/testCanonicalCharDrv
mkdir -p ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/layout
mkdir -p ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/values
mkdir -p ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/src/com/severs/android/directcharacterioaccess
mkdir -p ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io
mkdir -p ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/jni
mkdir -p ./aosp/hardware/libhardware/include/hardware
mkdir -p ./aosp/hardware/libhardware/modules/canonical_char_drv/java/com/severs/android/lib/charactor_io
mkdir -p ./aosp/hardware/libhardware/modules/canonical_char_drv/jni

# modified files
cp $AOSP_ROOTDIR/device/lge/mako/device.mk ./aosp/device/lge/mako/device.mk
cp $AOSP_ROOTDIR/device/lge/mako/init.mako.rc ./aosp/device/lge/mako/init.mako.rc

cp $AOSP_ROOTDIR/hardware/libhardware/modules/Android.mk ./aosp/hardware/libhardware/modules/Android.mk


# new files
cp $AOSP_ROOTDIR/device/lge/mako/bin/Android.mk ./aosp/device/lge/mako/bin/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/Android.mk ./aosp/device/lge/mako/custom_apps/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/Android.mk ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/AndroidManifest.xml ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/AndroidManifest.xml
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/layout/buffer_status.xml ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/layout/buffer_status.xml
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/values/colors.xml ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/values/colors.xml
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/values/strings.xml ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/res/values/strings.xml
cp $AOSP_ROOTDIR/device/lge/mako/custom_apps/DirectCharacterIOAccess/src/com/severs/android/directcharacterioaccess/DirectCharacterIOAccess.java ./aosp/device/lge/mako/custom_apps/DirectCharacterIOAccess/src/com/severs/android/directcharacterioaccess/DirectCharacterIOAccess.java

cp $AOSP_ROOTDIR/device/lge/mako/bin/testCanonicalCharDrv/Android.mk ./aosp/device/lge/mako/bin/testCanonicalCharDrv/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/bin/testCanonicalCharDrv/test_canonical_char_drv.c ./aosp/device/lge/mako/bin/testCanonicalCharDrv/test_canonical_char_drv.c

cp $AOSP_ROOTDIR/device/lge/mako/framework/Android.mk ./aosp/device/lge/mako/framework/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/Android.mk ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/java/Android.mk ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/LibCharactorIOException.java ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/LibCharactorIOException.java
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/LibCharactorIO.java ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/LibCharactorIO.java
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/Main.java ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/com/severs/android/lib/charactor_io/Main.java
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/java/com.severs.android.lib.charactor_io.xml ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/java/com.severs.android.lib.charactor_io.xml
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/jni/Android.mk ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/jni/Android.mk
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/jni/com_severs_android_lib_charactor_io_LibCharactorIO.cpp ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/jni/com_severs_android_lib_charactor_io_LibCharactorIO.cpp
cp $AOSP_ROOTDIR/device/lge/mako/framework/libcanonical_char_io_jni/jni/com_severs_android_lib_charactor_io_LibCharactorIO.h ./aosp/device/lge/mako/framework/libcanonical_char_io_jni/jni/com_severs_android_lib_charactor_io_LibCharactorIO.h

cp $AOSP_ROOTDIR/hardware/libhardware/include/hardware/canonical_char_drv.h ./aosp/hardware/libhardware/include/hardware/canonical_char_drv.h
cp $AOSP_ROOTDIR/hardware/libhardware/modules/canonical_char_drv/Android.mk ./aosp/hardware/libhardware/modules/canonical_char_drv/Android.mk
cp $AOSP_ROOTDIR/hardware/libhardware/modules/canonical_char_drv/libcanonical_char_drv.c ./aosp/hardware/libhardware/modules/canonical_char_drv/libcanonical_char_drv.c

