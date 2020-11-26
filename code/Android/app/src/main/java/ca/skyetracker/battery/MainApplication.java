/*
 *  Created by ClassicDIY on 25/11/20 6:55 AM
 *  Copyright (c) 2020 . All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

package ca.skyetracker.battery;

import android.app.Application;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;
import androidx.lifecycle.ProcessLifecycleOwner;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import com.google.gson.GsonBuilder;

import static androidx.lifecycle.Lifecycle.Event.ON_ANY;
import static androidx.lifecycle.Lifecycle.Event.ON_RESUME;
import static androidx.lifecycle.Lifecycle.Event.ON_START;
import static androidx.lifecycle.Lifecycle.Event.ON_STOP;

public class MainApplication extends Application implements LifecycleObserver {
    static Context context;
    static boolean isMQTTServiceBound = false;
    WifiManager.WifiLock wifiLock;
    private GsonBuilder gsonBuilder;
    MQTTService mqttService;
    public void onCreate() {
        super.onCreate();
        context = this.getBaseContext();
        ProcessLifecycleOwner.get().getLifecycle().addObserver(this);
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
