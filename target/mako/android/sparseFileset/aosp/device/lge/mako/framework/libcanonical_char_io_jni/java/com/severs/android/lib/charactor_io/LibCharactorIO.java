package com.severs.android.lib.charactor_io;

public class LibCharactorIO {

  private int nativeHandle;

  public LibCharactorIO() {
    this.init();
  }

  @Override
  protected void finalize() {
    this.close();
  }

  private native void init() throws LibCharactorIOException;
  public native void close();
  public native void flushBuffer() throws LibCharactorIOException;
  public native int getBufferCapacity() throws LibCharactorIOException;
  public native int getBufferSize() throws LibCharactorIOException;
  // TODO add timeout to writeBuffer
  public native int writeBuffer(byte[] buffer, int startByteIndex, int numBytes) throws LibCharactorIOException;
  public native byte[] readBuffer(int size, int timeoutInMs) throws LibCharactorIOException;
  public native int blockForAsyncInput(int timeoutInMs) throws LibCharactorIOException;;

  static {
     System.loadLibrary("charactor_io_jni");
  }
}
