LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include/
LOCAL_SRC_FILES := libcanonical_char_drv.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_MODULE := canonical_char_drv.default
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_CFLAGS += -g -O0
include $(BUILD_SHARED_LIBRARY)
