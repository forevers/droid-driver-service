LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := test_canonical_char_drv.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../hardware/libhardware/include/
LOCAL_SHARED_LIBRARIES := libhardware
LOCAL_CFLAGS += -g -O0 -std=c99
LOCAL_MODULE := test_canonical_char_drv
include $(BUILD_EXECUTABLE)
