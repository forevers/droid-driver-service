package com.severs.android.service.canonical;

/**
 * Canonical Listener API.
 *
 * {@hide}
 */
oneway interface ICanonicalListener {
  // TODO any time buffer state is modified by client or daemon issue a callback
  void onBufferStateChange(int size, int headIndex, int tailIndex);
}

