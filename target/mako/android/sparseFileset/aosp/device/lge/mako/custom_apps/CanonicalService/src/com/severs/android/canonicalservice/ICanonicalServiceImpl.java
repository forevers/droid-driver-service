package com.severs.android.canonicalservice;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Slog;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import com.severs.android.service.canonical.ICanonicalListener;
import com.severs.android.service.canonical.ICanonicalService;
import com.severs.android.lib.charactor_io.LibCharactorIO;
import com.severs.android.lib.charactor_io.LibCharactorIOException;

class ICanonicalServiceImpl extends ICanonicalService.Stub {

  private static final String TAG = "ICanonicalServiceImpl";
  private static final int INCREMENTAL_TIMEOUT = 2 * 1000;
  private static final boolean DEBUG = true;
  private final Map<IBinder, ListenerTracker> listeners = new HashMap<IBinder, ListenerTracker>();
  private final AtomicInteger stats = new AtomicInteger();
  private final Context context;
  private final LibCharactorIO libCharactorIO;
  private CanonicalServiceThread canonicalServiceThread;


  ICanonicalServiceImpl (Context context) {
    /* context will allow the service to enforce permissions */
    this.context = context;
    this.libCharactorIO = new LibCharactorIO();
  }


  protected void finalize () throws Throwable {
    this.libCharactorIO.close();
    super.finalize();
  }


  public void flushBuffer () {
    this.context.enforceCallingOrSelfPermission(
      Manifest.permission.CANONICAL_ACCESS, "canonical service access not permisable");
    if (DEBUG) Slog.d(TAG, "Flushing buffer.");
    this.libCharactorIO.flushBuffer();
    this.stats.incrementAndGet();
  }


  public int getBufferCapacity () {
    if (DEBUG) Slog.d(TAG, "Getting buffer capacity.");
    return this.libCharactorIO.getBufferCapacity();
  }


  public int getBufferSize () {
    if (DEBUG) Slog.d(TAG, "Getting buffer size.");
    return this.libCharactorIO.getBufferSize();
  }


  public int writeBuffer (byte[] buffer, int startByteIndex, int numBytes) {
    if (DEBUG) Slog.d(TAG, "Writing buffer.");
    return this.libCharactorIO.writeBuffer(buffer, startByteIndex, numBytes);
  }


  public byte[] readBuffer (int size, int timeoutInMs) {
    if (DEBUG) Slog.d(TAG, "Writing buffer.");
    return this.libCharactorIO.readBuffer(size, timeoutInMs);
  }


  public void register (ICanonicalListener listener) throws RemoteException {
    if (listener != null) {
      IBinder binder = listener.asBinder();
      synchronized(this.listeners) {
        if (this.listeners.containsKey(binder)) {
          Slog.w(TAG, "Ignoring duplicate listener: " + binder);
        } else {
          ListenerTracker listenerTracker = new ListenerTracker(listener);
          /* if client listener dies, remove it from the service's client listener map */
          binder.linkToDeath(listenerTracker, 0);
          this.listeners.put(binder, listenerTracker);
          if (DEBUG) Slog.d(TAG, "Registered listener: " + binder);
          if (this.canonicalServiceThread == null) {
            if (DEBUG) Slog.d(TAG, "Starting thread");
            this.canonicalServiceThread = new CanonicalServiceThread();
            this.canonicalServiceThread.start();
          }
        }
      }
    }
  }


  public void unregister (ICanonicalListener listener) {
    if (listener != null) {
      IBinder binder = listener.asBinder();
      synchronized(this.listeners) {
        ListenerTracker listenerTracker = this.listeners.remove(binder);
        if (listenerTracker == null) {
          Slog.w(TAG, "Ignoring unregistered listener: " + binder);
        } else {
          if (DEBUG) Slog.d(TAG, "Unregistered listener: " + binder);
          binder.unlinkToDeath(listenerTracker, 0);
          if (this.canonicalServiceThread != null && this.listeners.isEmpty()) {
            if (DEBUG) Slog.d(TAG, "Stopping thread");
            this.canonicalServiceThread.interrupt();
            this.canonicalServiceThread = null;
          }
        }
      }
    }
  }


  protected void dump (FileDescriptor fd, PrintWriter pw, String[] args) {
    if (this.context.checkCallingOrSelfPermission(android.Manifest.permission.DUMP) != PackageManager.PERMISSION_GRANTED) {
      pw.print("Permission Denial: can't dump ICanonicalService from from pid=");
      pw.print(Binder.getCallingPid());
      pw.print(", uid=");
      pw.println(Binder.getCallingUid());
      return;
    } else if (args.length > 0 && args[0] != null) {
      if (args[0].equals("stats")) {
        pw.println(this.stats.get());
      } else if (args[0].equals("size")) {
        pw.println(this.getBufferSize());
      } else if (args[0].equals("capacity")) {
        pw.println(this.getBufferCapacity());
      } else if (args[0].equals("listeners")) {
        pw.println(this.listeners.size());
      } else {
        pw.println("Usage: ICanonicalService [stats|size|capacity|listeners]");
      }
    } else {
      pw.println("ICanonicalServiceState:");
      pw.print("Statistics: ");
      pw.println(this.stats.get());
      pw.print("Size: ");
      pw.println(this.getBufferSize());
      pw.print("Capacity: ");
      pw.println(this.getBufferCapacity());
      pw.print("Listeners: ");
      pw.println(this.listeners.size());
    }
  }


  private final class ListenerTracker implements IBinder.DeathRecipient {
    private final ICanonicalListener listener;

    public ListenerTracker (ICanonicalListener listener) {
      this.listener = listener;
    }

    public ICanonicalListener getListener () {
      return this.listener;
    }

    /* unregister this lister from service map when client dies */
    public void binderDied () {
      ICanonicalServiceImpl.this.unregister(this.listener);
    }
  }


  /* TODO current driver implementation is a blocking (with timeout) ioctl call.
     may miss async events unless intermediate event queue is implemented. */
  private final class CanonicalServiceThread extends Thread {
    public void run () {

      while (!Thread.interrupted()) {
        try {

          if (DEBUG) Slog.d(TAG, "Waiting for buffer data");
          int new_size = ICanonicalServiceImpl.this.libCharactorIO.blockForAsyncInput(INCREMENTAL_TIMEOUT);
          if (new_size > 0) {
            if (DEBUG) Slog.d(TAG, "blockForAsyncInput() async input size: " + new_size);

            synchronized (ICanonicalServiceImpl.this.listeners) {
              for (Map.Entry<IBinder, ListenerTracker> entry : ICanonicalServiceImpl.this.listeners.entrySet()) {
                ICanonicalListener listener = entry.getValue().getListener();
                try {
                  if (DEBUG) Slog.d(TAG, "Notifying listener: " +  entry.getKey());
                  // TODO consider metadata dump from sysfs for headIndex, int tailIndex, ... in json or char/string blob */
                  int headIndex = -1;
                  int tailIndex = -1;
                  listener.onBufferStateChange(new_size, headIndex, tailIndex);
                } catch (RemoteException e) {
                  Slog.e(TAG, "Failed to update listener: " + entry.getKey(), e);
                  ICanonicalServiceImpl.this.unregister(listener);
                }
              }
            }

          } else {
            if (DEBUG) Slog.d(TAG, "blockForAsyncInput() timeout");
          }

        } catch (LibCharactorIOException e) {
          Slog.e(TAG, "Canonical Async blocking failure", e);
          try {
            Thread.sleep(INCREMENTAL_TIMEOUT);
          } catch (InterruptedException e2) {
            break;
          }
        }
      }
    }
  }
}
