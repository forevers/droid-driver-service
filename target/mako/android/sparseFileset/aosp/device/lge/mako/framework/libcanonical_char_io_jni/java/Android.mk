LOCAL_PATH := $(call my-dir)

# Build the library
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := com.severs.android.lib.charactor_io
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := $(call all-java-files-under,.)
include $(BUILD_JAVA_LIBRARY)

# Build the documentation
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(call all-subdir-java-files) $(call all-subdir-html-files)
LOCAL_MODULE:= com.severs.android.lib.charactor_io_doc
LOCAL_DROIDDOC_OPTIONS := com.severs.android.lib.charactor_io
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_DROIDDOC_USE_STANDARD_DOCLET := true
include $(BUILD_DROIDDOC)

# Copy com.severs.android.lib.charactor_io.xml to /system/etc/permissions/
# http://karthiksden.blogspot.com/2011/03/copying-data-files-using-androidmk.html
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := com.severs.android.lib.charactor_io.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
