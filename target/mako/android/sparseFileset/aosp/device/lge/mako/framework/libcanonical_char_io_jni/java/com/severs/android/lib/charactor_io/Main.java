package com.severs.android.lib.charactor_io;

// import android.util.Log;

/** @hide */
public class Main {

  static final int WRITE_CHUNK_SIZE = 10;
  static final int READ_CHUNK_SIZE = WRITE_CHUNK_SIZE;

  public static void main (String[] args) {
    try {
      LibCharactorIO libCharactorIO = new LibCharactorIO();
      System.out.printf("open() success\n");

      try {
        // Log.i("Main", "libCharactorIO try");

        // buffer charactoristics
        int size = libCharactorIO.getBufferSize();
        int capacity = libCharactorIO.getBufferCapacity();
        System.out.printf("size: %d, capacity: %d\n", size, capacity);

        // buffer read with 5 second timeout
        System.out.printf("wait_for_buffer_data()\n");
        byte[] buffer = libCharactorIO.waitForBufferData(5000);
        if (buffer.length == 0) {
            System.out.printf("timeout waiting for data\n");
        } else {
            System.out.printf("read %d bytes\n", buffer.length);
        }

        // write data to buffer
        byte[] writeBuffer = new byte[WRITE_CHUNK_SIZE];
        for (int i = 0; i < WRITE_CHUNK_SIZE; ++i) {
          writeBuffer[i] = (byte)i;
        }
        int bytesWritten = libCharactorIO.writeBuffer(writeBuffer, 0, WRITE_CHUNK_SIZE);
        System.out.printf("bytes written: %d\n", bytesWritten);
        size = libCharactorIO.getBufferSize();
        System.out.printf("post write buffer size: %d\n", size);

        libCharactorIO.flushBuffer();
        size = libCharactorIO.getBufferSize();
        System.out.printf("post flush buffer size: %d\n", size);

      } finally {
        System.out.printf("libCharactorIO finally\n");
        //libCharactorIO.close();
      }
    } catch (LibCharactorIOException e) {
        System.err.println("LibCharactorIO access error\n");
        e.printStackTrace();
    }
  }
}
