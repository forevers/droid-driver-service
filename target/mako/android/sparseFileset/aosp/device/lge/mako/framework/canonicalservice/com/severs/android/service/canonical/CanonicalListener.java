package com.severs.android.service.canonical;

/**
 * Callbacks for buffer state changes.
 */
public interface CanonicalListener {

  /**
   * @param size buffer size.
   * @param headIndex buffer head index.
   * @param tailIndex buffer tail index.
   */
  public void onBufferStateChange(int size, int headIndex, int tailIndex);
}

