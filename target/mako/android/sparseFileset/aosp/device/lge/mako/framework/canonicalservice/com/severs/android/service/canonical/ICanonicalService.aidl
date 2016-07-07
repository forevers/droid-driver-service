package com.severs.android.service.canonical;

import com.severs.android.service.canonical.ICanonicalListener;

/**
 * Canonical Service API.
 *
 * {@hide}
 */
interface ICanonicalService {
// TODO example of buffer transfer API qualification in/out/inout
  void flushBuffer();
  int getBufferCapacity();
  int getBufferSize();
  int writeBuffer(in byte[] buffer, int startByteIndex, int numBytes);
  byte[] readBuffer(in int size, int timeoutInMs);
  void register(in ICanonicalListener listener);
  void unregister(in ICanonicalListener listener);
}
