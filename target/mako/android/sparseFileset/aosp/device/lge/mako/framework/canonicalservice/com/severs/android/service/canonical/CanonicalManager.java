package com.severs.android.service.canonical;

import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.util.Slog;
import java.util.HashSet;
import java.util.Set;

public class CanonicalManager {
  private static final String TAG = "CanonicalManager";
  private static final String REMOTE_SERVICE_NAME = ICanonicalService.class.getName();
  private static final boolean DEBUG = true; //false;

  /* Client may register many listeners with the manager. Manager registers only
     one listener with the service. */
  // TODO ConcurrentHashSet ?
  private final Set<CanonicalListener> listeners = new HashSet<CanonicalListener>();

  /* message class */
  private static final int MESSAGE_METADATA = 0;
  private class BufferStateMetadata {

    public int mSize;
		public int mHeadIndex;
    public int mTailIndex;

    private BufferStateMetadata(int size, int headIndex, int tailIndex ) {
    	mSize = size;
			mHeadIndex = headIndex;
    	mTailIndex = tailIndex;
		}
  }

  /* Binder thread listener callback */
  private final ICanonicalListener canonicalListener = new ICanonicalListener.Stub() {
		public void onBufferStateChange(int size, int headIndex, int tailIndex) {

      if (DEBUG) Slog.d(TAG, "onBufferStateChange: " + size + ":" + headIndex + ":" + tailIndex);
      
      BufferStateMetadata metadata = new BufferStateMetadata(size, headIndex, tailIndex);
      /* obtain message from handler pool and send to looper */  
      CanonicalManager.this.handler.obtainMessage(MESSAGE_METADATA, metadata).sendToTarget();
		}
  };

  /* UI thread listener callback(s) */
  private final Handler handler = new Handler() {
    @Override
    public void handleMessage(Message message) {

			if (message.what == MESSAGE_METADATA) {
				BufferStateMetadata metadata = (BufferStateMetadata)message.obj;

		    if (DEBUG) Slog.d(TAG, "Notifying local listeners: " + metadata.mSize + ":" + metadata.mHeadIndex + ":" + metadata.mTailIndex);
		    synchronized(CanonicalManager.this.listeners) {
		      for (CanonicalListener canonicalListener : CanonicalManager.this.listeners) {
		        if (DEBUG) Slog.d(TAG, "Notifying local listener [" + canonicalListener
		          + "] of more used data: " + metadata.mSize + ":" + metadata.mHeadIndex + ":" + metadata.mTailIndex);
		        canonicalListener.onBufferStateChange(metadata.mSize, metadata.mHeadIndex, metadata.mTailIndex);
		      }
		    }
			}
    }
  };

  private final ICanonicalService service;

  public static CanonicalManager getInstance() {
    return new CanonicalManager();
  }

  private CanonicalManager() {
    Log.d(TAG, "Connecting to ICanonicalService by name [" + REMOTE_SERVICE_NAME + "]");
    this.service = ICanonicalService.Stub.asInterface(ServiceManager.getService(REMOTE_SERVICE_NAME));
    if (this.service == null) {
      throw new IllegalStateException("Failed to find ICanonicalService by name [" + REMOTE_SERVICE_NAME + "]");
    }
  }

  public void flushBuffer() {
    try {
      if (DEBUG) Slog.d(TAG, "Flushing buffer.");
      this.service.flushBuffer();
    } catch (RemoteException e) {
      throw new RuntimeException("Failed to flush buffer", e);
    }
  }

  public int getBufferCapacity() {
    try {
      if (DEBUG) Slog.d(TAG, "Getting buffer capacity.");
      return this.service.getBufferCapacity();
    } catch (RemoteException e) {
      throw new RuntimeException("Failed to get buffer capacity", e);
    }
  }

  public int getBufferSize() {
    try {
      if (DEBUG) Slog.d(TAG, "Getting buffer size.");
      return this.service.getBufferSize();
    } catch (RemoteException e) {
      throw new RuntimeException("Failed to get buffer size", e);
    }
  }

	public void writeBuffer(byte[] buffer, int startByteIndex, int numBytes) {
    try {
      if (DEBUG) Slog.d(TAG, "Writing buffer.");
      this.service.writeBuffer(buffer, startByteIndex, numBytes);
    } catch (RemoteException e) {
      throw new RuntimeException("Failed to write buffer", e);
    }
	}

  public byte[] readBuffer(int size, int timeoutInMs) {
    try {
      if (DEBUG) Slog.d(TAG, "Reading buffer.");
      return this.service.readBuffer(size, timeoutInMs);
    } catch (RemoteException e) {
      throw new RuntimeException("Failed to read buffer", e);
    }
	}

  public void register(CanonicalListener listener) {
    if (listener != null) {
      synchronized(this.listeners) {
        if (this.listeners.contains(listener)) {
          Log.w(TAG, "Already registered: " + listener);
        } else {
          try {
            boolean registerRemote = this.listeners.isEmpty();
            if (DEBUG) Log.d(TAG, "Registering local listener.");
            this.listeners.add(listener);
            if (registerRemote) {
              if (DEBUG) Log.d(TAG, "Registering remote listener.");
              this.service.register(this.canonicalListener);
            }
          } catch (RemoteException e) {
            throw new RuntimeException("Failed to register " + listener, e);
          }
        }
      }
    }
  }

  public void unregister(CanonicalListener listener) {
    if (listener != null) {
      synchronized(this.listeners) {
        if (!this.listeners.contains(listener)) {
          Log.w(TAG, "Not registered: " + listener);
        } else {
          if (DEBUG) Log.d(TAG, "Unregistering local listener.");
          this.listeners.remove(listener);
        }
        if (this.listeners.isEmpty()) {
          try {
            if (DEBUG) Log.d(TAG, "Unregistering remote listener.");
            this.service.unregister(this.canonicalListener);
          } catch (RemoteException e) {
            throw new RuntimeException("Failed to unregister " + listener, e);
          }
        }
      }
    }
  }

}

