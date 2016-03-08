package com.severs.android.canonicalservice;

import android.app.Application;
import android.os.ServiceManager;
import android.util.Log;
import com.severs.android.service.canonical.ICanonicalService;


public class CanonicalServiceApp extends Application {
  private static final String TAG = "CanonicalServiceApp";
  private static final String REMOTE_SERVICE_NAME = ICanonicalService.class.getName();
  private ICanonicalServiceImpl serviceImpl;

  public void onCreate() {
    super.onCreate();
    this.serviceImpl = new ICanonicalServiceImpl(this);
    ServiceManager.addService(REMOTE_SERVICE_NAME, this.serviceImpl);
    Log.d(TAG, "Registered [" + serviceImpl.getClass().getName() + "] as [" + REMOTE_SERVICE_NAME + "]");
  }

  public void onTerminate() {
    super.onTerminate();
    Log.d(TAG, "Terminated");
  }
}

