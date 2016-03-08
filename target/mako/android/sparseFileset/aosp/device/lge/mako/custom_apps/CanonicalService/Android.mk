LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under,src)
LOCAL_REQUIRED_MODULES := \
    com.severs.android.service.canonical \
    com.severs.android.lib.charactor_io
# framework allows service to access the hidden ServiceManager
LOCAL_JAVA_LIBRARIES := \
    com.severs.android.service.canonical \
    com.severs.android.lib.charactor_io \
    core \
    framework
LOCAL_PACKAGE_NAME := CanonicalService
LOCAL_SDK_VERSION := current
LOCAL_PROGUARD_ENABLED := disabled
# platform allows service to run as system user to access ServiceManager
LOCAL_CERTIFICATE := platform
include $(BUILD_PACKAGE)
