#!/bin/bash

# create sparse directory of modified and newly created files

# android kernel location

KERNEL_ROOTDIR=~/aospDevelopment/kernelMako/msm

# modified files
mkdir -p ./msm/android/configs
mkdir -p ./msm/arch/arm/configs
mkdir -p ./msm/drivers
mkdir -p ./msm/drivers/staging/android

cp $KERNEL_ROOTDIR/android/configs/android-base.cfg ./msm/android/configs/android-base.cfg
cp $KERNEL_ROOTDIR/arch/arm/configs/mako_defconfig ./msm/arch/arm/configs/mako_defconfig
cp $KERNEL_ROOTDIR/drivers/Kconfig ./msm/drivers/Kconfig
cp $KERNEL_ROOTDIR/drivers/Makefile ./msm/drivers/Makefile
cp $KERNEL_ROOTDIR/drivers/staging/android/Kconfig ./msm/drivers/staging/android/Kconfig
cp $KERNEL_ROOTDIR/drivers/staging/android/Makefile ./msm/drivers/staging/android/Makefile

# new files
mkdir -p ./msm/drivers
mkdir -p ./msm/drivers/canonical_char_drv

cp $KERNEL_ROOTDIR/drivers/canonical_char_drv/canonical_char_drv.c ./msm/drivers/canonical_char_drv/canonical_char_drv.c
cp $KERNEL_ROOTDIR/drivers/canonical_char_drv/Kconfig ./msm/drivers/canonical_char_drv/Kconfig
cp $KERNEL_ROOTDIR/drivers/canonical_char_drv/Makefile ./msm/drivers/canonical_char_drv/Makefile
cp $KERNEL_ROOTDIR/drivers/Makefile ./msm/drivers/Makefile
