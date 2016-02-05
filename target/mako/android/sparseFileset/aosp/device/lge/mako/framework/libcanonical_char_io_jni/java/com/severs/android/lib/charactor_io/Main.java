package com.severs.android.lib.charactor_io;

// import android.util.Log;

/** @hide */
public class Main {

  static final int WRITE_CHUNK_SIZE = 10;
  static final int READ_CHUNK_SIZE = 5;

  private static byte[] writeBuffer;
  private static byte[] readBuffer;

  public static void main (String[] args) {

    try {
      LibCharactorIO libCharactorIO = new LibCharactorIO();
      System.out.printf("open() success\n");

      try {
        // Log.i("Main", "libCharactorIO try");
        int timeoutInMs = 5000;

        // buffer charactoristics
        int size = libCharactorIO.getBufferSize();
        int capacity = libCharactorIO.getBufferCapacity();
        System.out.printf("size: %d, capacity: %d\n", size, capacity);

        if (size >= 0 && capacity >= 0) {

          int bytesWritten;

          /* write buffer allocation */
          byte[] writeBuffer = new byte[WRITE_CHUNK_SIZE];

          /* buffer read all with 5 second timeout */
          System.out.printf("wait_for_buffer_data()\n");
          byte[] buffer = libCharactorIO.readBuffer(-1, 5000);
          if (buffer.length == 0) {
              System.out.printf("timeout waiting for data\n");
          } else {
              System.out.printf("read %d bytes\n", buffer.length);
          }

          /* write data */
          for (int i = 0; i < WRITE_CHUNK_SIZE; ++i) {
            writeBuffer[i] = (byte)i;
          }
          bytesWritten = libCharactorIO.writeBuffer(writeBuffer, 0, WRITE_CHUNK_SIZE);
          System.out.printf("bytes written: %d\n", bytesWritten);
          size = libCharactorIO.getBufferSize();
          System.out.printf("post write buffer size: %d\n", size);

          /* read data */
          readBuffer = libCharactorIO.readBuffer(READ_CHUNK_SIZE, timeoutInMs);
          System.out.printf("read %d bytes\n", readBuffer.length);
          for (int i = 0; i < readBuffer.length; ++i) {
            System.out.printf("  0x%x", readBuffer[i]);
          }
          System.out.printf("\n");

          /* read data */
          readBuffer = libCharactorIO.readBuffer(2, timeoutInMs);
          System.out.printf("read %d bytes\n", readBuffer.length);
          for (int i = 0; i < readBuffer.length; ++i) {
            System.out.printf("  0x%x", readBuffer[i]);
          }
          System.out.printf("\n");

          /* read data */
          readBuffer = libCharactorIO.readBuffer(WRITE_CHUNK_SIZE, timeoutInMs);
          System.out.printf("read %d bytes\n", readBuffer.length);
          for (int i = 0; i < readBuffer.length; ++i) {
            System.out.printf("  0x%x", readBuffer[i]);
          }
          System.out.printf("\n");

          /* write data */
          bytesWritten = libCharactorIO.writeBuffer(writeBuffer, 0, WRITE_CHUNK_SIZE);
          System.out.printf("bytes written: %d\n", bytesWritten);
          size = libCharactorIO.getBufferSize();
          System.out.printf("post write buffer size: %d\n", size);

          /* read data */
          readBuffer = libCharactorIO.readBuffer(READ_CHUNK_SIZE, timeoutInMs);
          System.out.printf("read %d bytes\n", readBuffer.length);
          for (int i = 0; i < readBuffer.length; ++i) {
            System.out.printf("  0x%x", readBuffer[i]);
          }
          System.out.printf("\n");
          
          /* flush the buffer */
          libCharactorIO.flushBuffer();

          /* get current size */
          size = libCharactorIO.getBufferSize();
          System.out.printf("post flush buffer size: %d\n", size);

        } else {
          System.out.printf("size < 0 OR capacity < 0\n");
        }

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
