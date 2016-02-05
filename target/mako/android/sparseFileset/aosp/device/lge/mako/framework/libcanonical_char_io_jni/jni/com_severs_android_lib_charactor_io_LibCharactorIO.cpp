#include <jni.h>
//#include <canonical_char_io_hal_lib.h> remove this redunant header
// #include <hardware/hardware.h>
#include <android/log.h>
#include <hardware/canonical_char_drv.h>
#include "JNIHelp.h"

#include <errno.h>

static void throwLibCharactorIOException(JNIEnv *env, const char *msg) {
  jniThrowException(env, "com/severs/android/lib/charactor_io/LibCharactorIOException", msg);
}

static jfieldID nativeHandleFieldId;


static void native_init (JNIEnv *env, jobject object) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "native_init()");
  hw_module_t* module;
  int err = hw_get_module(CANONICAL_CHR_DRV_MODULE_ID, (hw_module_t const**)&module);
  if (err) {
    throwLibCharactorIOException(env, "Failed to get module");
  } else {
    struct canonical_char_drv_device_t *dev;
    err = module->methods->open(module, 0, (struct hw_device_t **) &dev);
    if (err) {
      throwLibCharactorIOException(env, "Failed to open device");
    } else {
      env->SetIntField(object, nativeHandleFieldId, (jint) dev);
    }
  }
}


static struct canonical_char_drv_device_t * getDevice (JNIEnv *env, jobject object) {
  return (struct canonical_char_drv_device_t *) env->GetIntField(object, nativeHandleFieldId);
}


static void native_close (JNIEnv *env, jobject object) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "native_close()");

  struct canonical_char_drv_device_t *dev = getDevice(env, object);
  dev->common.close((struct hw_device_t *)dev);
}


static void flushBuffer (JNIEnv *env, jobject object) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "flushBuffer()");
  struct canonical_char_drv_device_t *dev = getDevice(env, object);
  if (dev->flush_buffer(dev) != 0) {
    throwLibCharactorIOException(env, "Failed to flush buffer");
  }
}


static jint getBufferCapacity (JNIEnv *env, jobject object) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "getBufferCapacity()");
  struct canonical_char_drv_device_t *dev = getDevice(env, object);
  int ret = dev->get_buffer_capacity(dev);

  if (ret < 0) {
    throwLibCharactorIOException(env, "Failed to get buffer size");
  }
  return ret;
}


static jint getBufferSize (JNIEnv *env, jobject object) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "getBufferSize()");
  struct canonical_char_drv_device_t *dev = getDevice(env, object);
  int ret = dev->get_buffer_size(dev);
  if (ret < 0) {
    throwLibCharactorIOException(env, "Failed to get used log size");
  }
  return ret;
}


static jbyteArray readBuffer (JNIEnv *env, jobject object, jint size, jint timeoutInMs) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "readBuffer()");

  struct canonical_char_drv_device_t *dev = getDevice(env, object); 

  int current_buffer_size = getBufferSize(env, object);

  if (size == -1) {
    /* get total buffer size */
    size = current_buffer_size;
    // TODO check for assertion
  }

  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "size: %d", size);

  // trim size if needed
  size = (size <= current_buffer_size) ? size : current_buffer_size;

  /* allocate buffer */
  jbyteArray jbyteBuffer = env->NewByteArray(size);

  /* fill buffer */
  if (size > 0) {
    jboolean isCopy;
    jbyte* pBuffer = env->GetByteArrayElements(jbyteBuffer, &isCopy);
    int ret = dev->wait_for_buffer_data(dev, reinterpret_cast<uint8_t*>(pBuffer), size, timeoutInMs);
    if (ret >= 0) {
      env->ReleaseByteArrayElements(jbyteBuffer, pBuffer, 0);
    } else {
      throwLibCharactorIOException(env, "Failed while waiting for buffer data");
    }
  }

  return jbyteBuffer;
}

static jint writeBuffer (JNIEnv *env, jobject object, jbyteArray jbyteBuffer, jint startByteIndex, jint numBytes) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "writeBuffer()");

  struct canonical_char_drv_device_t *dev = getDevice(env, object);

  jboolean isCopy;
  jbyte* pWriteBuffer = env->GetByteArrayElements(jbyteBuffer, &isCopy);
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "  a new copy is created: %d", isCopy);

  int bytesWritten = 0;
  // TODO review casting
  if ((bytesWritten = dev->write_buffer(dev, reinterpret_cast<uint8_t*>(pWriteBuffer+startByteIndex), numBytes)) >= 0) {
    __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "num bytes written: %d", bytesWritten);
  } else {
      __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "Failed to write buffer: %s", strerror(errno));
      throwLibCharactorIOException(env, "Failed while waiting for buffer data");
  }

  env->ReleaseByteArrayElements(jbyteBuffer, pWriteBuffer, 0);

  return bytesWritten;
}


static JNINativeMethod method_table[] = {
  { "init", "()V", (void *) native_init },
  { "close", "()V", (void *) native_close },
  { "flushBuffer", "()V", (void *) flushBuffer },
  { "getBufferCapacity", "()I", (void *) getBufferCapacity },
  { "getBufferSize", "()I", (void *) getBufferSize },
  { "readBuffer", "(II)[B", (void *) readBuffer },
  { "writeBuffer", "([BII)I", (void*) writeBuffer }
};

static const char * class_name = "com/severs/android/lib/charactor_io/LibCharactorIO";

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "JNI_OnLoad()");
  JNIEnv* env = NULL;
  if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
    __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "JNI_VERSION_1_4");
    if (jniRegisterNativeMethods(env, class_name, method_table, NELEM(method_table)) == 0) {
      __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "jniRegisterNativeMethods()");
      jclass clazz = env->FindClass(class_name);
      if (clazz) {
        __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "clazz");
        nativeHandleFieldId = env->GetFieldID(clazz, "nativeHandle", "I");
        if (nativeHandleFieldId) {
          __android_log_print(ANDROID_LOG_INFO, "charactor_io_jni", "nativeHandleFieldId");
          return JNI_VERSION_1_4;
        }
      }
    }
  }
  return JNI_ERR;
}
