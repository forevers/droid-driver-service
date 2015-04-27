#!/bin/bash

# create sparse directory of modified and newly created files

# modified files
mkdir -p sparseFileset/msm/android/configs
mkdir -p sparseFileset/msm/arch/arm/configs
mkdir -p sparseFileset/msm/drivers
mkdir -p sparseFileset/msm/drivers/staging/android

cp msm/android/configs/android-base.cfg sparseFileset/msm/android/configs/android-base.cfg
cp msm/arch/arm/configs/mako_defconfig sparseFileset/msm/arch/arm/configs/mako_defconfig
cp msm/drivers/Kconfig sparseFileset/msm/drivers/Kconfig
cp msm/drivers/Makefile sparseFileset/msm/drivers/Makefile
cp msm/drivers/staging/android/Kconfig sparseFileset/msm/drivers/staging/android/Kconfig
cp msm/drivers/staging/android/Makefile sparseFileset/msm/drivers/staging/android/Makefile

# new files
mkdir -p sparseFileset/msm/drivers
mkdir -p sparseFileset/msm/drivers/canonical_char_drv

cp msm/drivers/canonical_char_drv/canonical_char_drv.c sparseFileset/msm/drivers/canonical_char_drv/canonical_char_drv.c
cp msm/drivers/canonical_char_drv/Kconfig sparseFileset/msm/drivers/canonical_char_drv/Kconfig
cp msm/drivers/canonical_char_drv/Makefile sparseFileset/msm/drivers/canonical_char_drv/Makefile
cp msm/drivers/Makefile sparseFileset/msm/drivers/Makefile
