package ca.skyetracker.battery;

import android.app.Application;
import android.arch.lifecycle.LifecycleObserver;
import android.arch.lifecycle.LifecycleOwner;
import android.arch.lifecycle.OnLifecycleEvent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import com.google.gson.GsonBuilder;

import static android.arch.lifecycle.Lifecycle.Event.ON_RESUME;
import static android.arch.lifecycle.Lifecycle.Event.ON_START;
import static android.arch.lifecycle.Lifecycle.Event.ON_STOP;

/**
 * Created by Me on 10/21/2017.
 */

public class MainApplication extends Application implements LifecycleObserver {
    static Context context;
    static boolean isMQTTServiceBound = false;
    WifiManager.WifiLock wifiLock;
    private GsonBuilder gsonBuilder;
    MQTTService mqttService;
    public void onCreate() {
        super.onCreate();
        context = this.getBaseContext();
        bindService(new Intent(this, MQTTService.class), mqttServiceConnection, Context.BIND_AUTO_CREATE);
    }

    private ServiceConnection mqttServiceConnection = new ServiceConnection() {

        public void onServiceConnected(ComponentName className, IBinder service) {
            MQTTService.MQTTServiceBinder binder = (MQTTService.MQTTServiceBinder) service;
            mqttService = binder.getService();
            isMQTTServiceBound = true;
            mqttService.monitorBattery();
            Log.d(getClass().getName(), "MQTTService ServiceConnected");
        }

        public void onServiceDisconnected(ComponentName arg0) {
            isMQTTServiceBound = false;
            mqttService = null;
            Log.d(getClass().getName(), "MQTTService ServiceDisconnected");
        }
    };

    @OnLifecycleEvent(ON_START)
    void onStart(LifecycleOwner source) {
        Log.d(getClass().getName(), "onStart");
        if (wifiLock != null) {
            wifiLock.acquire();
        }
        bindService(new Intent(this, MQTTService.class), mqttServiceConnection, Context.BIND_AUTO_CREATE);
        Log.d(getClass().getName(), "onStart Done");
    }

    @OnLifecycleEvent(ON_STOP)
    void onStop(LifecycleOwner source) {
        Log.d(getClass().getName(), "onStop");
        if (wifiLock != null) {
            wifiLock.release();
        }
//        SaveSettings();
        try {
            if (isMQTTServiceBound && mqttService != null) {
                mqttService.stopMonitoringBattery();
            }
        } catch (Exception e) {
            Log.w(getClass().getName(), "stop service exception");
            e.printStackTrace();
        }
        Log.d(getClass().getName(), "onStop Done");
    }

    @OnLifecycleEvent(ON_RESUME)
    void onResume(LifecycleOwner source) {
        Log.d(getClass().getName(), "onResume");

    }
}
