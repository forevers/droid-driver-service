LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := com_severs_android_lib_charactor_io_LibCharactorIO.cpp
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE) $(LOCAL_PATH)/../../../include/
LOCAL_CFLAGS += -g -O0
LOCAL_SHARED_LIBRARIES := libhardware libnativehelper liblog
LOCAL_MODULE := libcharactor_io_jni
include $(BUILD_SHARED_LIBRARY)
