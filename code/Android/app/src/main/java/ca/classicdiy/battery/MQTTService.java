
/*
 *  Created by ClassicDIY on 26/11/20 9:53 AM
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

package ca.classicdiy.battery;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import androidx.preference.PreferenceManager;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.util.Timer;

public class MQTTService extends Service {
    private final IBinder mBinder = new MQTTServiceBinder();
    private MqttAndroidClient mqttClient;
    private Timer mqttWakeTimer;
    private GsonBuilder gsonBuilder;
    private String commandTopic = "";
    private String statusTopic = "";
    private String telemetryTopic = "";

    public MQTTService() {

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent arg0) {
        Log.d(getClass().getName(), "onBind");
        return mBinder;
    }


    public class MQTTServiceBinder extends Binder {
        MQTTService getService() {
            return MQTTService.this;
        }
    }

    @Override
    public void onCreate() {
        Log.d(getClass().getName(), "onCreate");
        super.onCreate();
        gsonBuilder = new GsonBuilder();
        gsonBuilder.registerTypeAdapterFactory(new BundleTypeAdapterFactory());
        getBaseContext().registerReceiver(cmndReceiver, new IntentFilter(Constants.COMMAND_SUBTOPIC));
    }

    private void BroadcastMessage(Bundle data, String action) {
        Intent commandIntent = new Intent().setAction(action);
        commandIntent.putExtras(data);
        this.sendBroadcast(commandIntent);
    }

    //    Our handler for received Intents.
    private BroadcastReceiver cmndReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
        if (mqttClient != null && mqttClient.isConnected()) {
            try {
                if ("publish".equals(intent.getIdentifier())) {
                    mqttClient.publish(String.format("%s/operation", commandTopic), intent.getStringExtra("operation").getBytes(), 0, false);
                }
            } catch (MqttException e) {
                e.printStackTrace();
            }
        }
        }
    };

    private void BroadcastToast(String message) {
        Intent intent2 = new Intent(Constants.CA_BATTERY_TOAST);
        intent2.putExtra("message", message);
        getApplicationContext().sendBroadcast(intent2);
    }

    @Override
    public void onDestroy() {
        Log.d(getClass().getName(), "onDestroy");
        try {
            mqttClient.disconnect();
        } catch (MqttException e) {
            e.printStackTrace();
        }
        getBaseContext().unregisterReceiver(cmndReceiver);
        super.onDestroy();
    }

    SharedPreferences.OnSharedPreferenceChangeListener prefListener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
                public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
                    if (key.equals("MQTTHost")) {
                        //ToDo reconnect on change
                    }
                }
            };

    public void stopMonitoringBattery() {
        try {
            UnSubscribe();
            if (mqttClient != null && mqttClient.isConnected()) {
                IMqttToken token = mqttClient.disconnect();
                Log.d(getClass().getName(), "mqttClient disconnected");
            }
            SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
            sharedPreferences.unregisterOnSharedPreferenceChangeListener(prefListener);
        } catch (Exception e) {
            Log.w(getClass().getName(), "unSubscribe exception");
            e.printStackTrace();
        }
    }

    public void monitorBattery() {
        Log.d(getClass().getName(), "monitorBattery");
        {
            try {
                SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
                sharedPreferences.registerOnSharedPreferenceChangeListener(prefListener);
                if (sharedPreferences.contains("MQTTHost")) { // configured?
                    connectToMQTT(sharedPreferences);
                }
            } catch (MqttException e) {
                Log.w(getClass().getName(), "connectToMQTT exception");
                e.printStackTrace();
            }
        }
    }

    private boolean connectToMQTT(SharedPreferences sharedPreferences) throws MqttException {
        boolean rVal = false;
        try {
            if (mqttClient != null) {
                rVal = mqttClient.isConnected();
            }
        } catch (Exception ex) {
            mqttClient = null;
        }
        if (rVal == false) {
            String ip = sharedPreferences.getString("MQTTHost", "192.168.86.25");
            String port = sharedPreferences.getString("MQTTPort", "1883");
            Log.d(getClass().getName(), "connectToMQTT");
            String brokerUrl = String.format("tcp://%s:%s", ip, port);
            if (mqttClient == null) {
                String clientId = Constants.TAG + System.currentTimeMillis() * 1000000L;
                mqttClient = new MqttAndroidClient(getApplicationContext(), brokerUrl, clientId);
                SetupMQTTCallback();
                Log.d(getClass().getName(), "creating new mqttClient");
            }
            MqttConnectOptions mqttConnectOptions = new MqttConnectOptions();
            mqttConnectOptions.setCleanSession(true);

            String user = sharedPreferences.getString("MQTTUser", "argon");
            mqttConnectOptions.setUserName(user);
            String pw = sharedPreferences.getString("MQTTPassword", "volvo4");
            mqttConnectOptions.setPassword(pw.toCharArray());
            commandTopic = String.format("%s/%s", sharedPreferences.getString("TesterGroup", "Battery"), "cmnd");
            statusTopic = String.format("%s/%s", sharedPreferences.getString("TesterGroup", "Battery"), "stat");
            telemetryTopic = String.format("%s/%s", sharedPreferences.getString("TesterGroup", "Battery"), "tele");
            IMqttToken token = mqttClient.connect(mqttConnectOptions);
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    Subscribe();
                    Log.d(getClass().getName(), "mqttClient connected");
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.w(getClass().getName(), String.format("mqttClient failed to connect: %s", exception.getMessage()));
                    BroadcastToast("Failed to connect to MQTT broker");
                }
            });
            rVal = true;
        }
        return rVal;
    }

    private void Subscribe() {
        Log.d(getClass().getName(), "Subscribe to MQTT ");
        if (mqttClient.isConnected()) {
            try {

                SubscribeTo(commandTopic);
                SubscribeTo(statusTopic);
            } catch (Exception e) {
                Log.w(getClass().getName(), e.getMessage());
                e.printStackTrace();
            }
        }
    }

    private void SubscribeTo(String topic) throws MqttException {
        Log.d(getClass().getName(), "SubscribeTo " + topic);
        IMqttToken token = mqttClient.subscribe(String.format("%s/#", topic), 1);
        token.setActionCallback(new IMqttActionListener() {
            @Override
            public void onSuccess(IMqttToken iMqttToken) {
                Log.d(getClass().getName(), "Subscribe Successful " + (iMqttToken.getTopics().length > 0 ? iMqttToken.getTopics()[0] : ""));
            }

            @Override
            public void onFailure(IMqttToken iMqttToken, Throwable throwable) {
                Log.w(getClass().getName(), "Subscribe Failed " + iMqttToken.getException().getMessage());
                BroadcastToast("Failed to subscribe: " + iMqttToken.getException().getMessage());
            }
        });
    }

    private void SetupMQTTCallback() {
        mqttClient.setCallback(new MqttCallback() {

            @Override
            public void connectionLost(Throwable throwable) {

                Log.w(getClass().getName(), String.format("Lost MQTT connection: %s", throwable.getMessage()));
            }

            @Override
            public void messageArrived(String topic, MqttMessage mqttMessage) throws Exception {

                try {
                    String str = mqttMessage.toString();
                    Gson gson = gsonBuilder.create();
                    String[] elements = topic.split("/");
                    Log.d(getClass().getName(), "MQTT messageArrived: " + topic + "|" + str);
                    Intent intent = new Intent();
                    if (topic.startsWith(statusTopic)) {
                        intent.setAction(Constants.STATUS_SUBTOPIC);
                        if (topic.endsWith("monitor")) {
                            Bundle b = gson.fromJson(str, Bundle.class);
                            b.putBundle("monitor", gson.fromJson(str, Bundle.class));
                            intent.putExtras(b);
                            intent.setIdentifier("monitor");
                            sendBroadcast(intent);
                        }
                        else if (topic.endsWith("result")) {
                            Bundle b = gson.fromJson(str, Bundle.class);
                            b.putBundle("result", gson.fromJson(str, Bundle.class));
                            intent.putExtras(b);
                            intent.setIdentifier("result");
                            sendBroadcast(intent);
                        }
                        else if (topic.endsWith("mode")) {
                            Bundle b = new Bundle();
                            b.putString("mode", str);
                            intent.putExtras(b);
                            intent.setIdentifier("mode");
                            sendBroadcast(intent);
                        }

                    }
                    else if (topic.startsWith(commandTopic)) {
                        intent.setAction(Constants.COMMAND_SUBTOPIC);
                        if (topic.endsWith("operation")) {
                            Bundle b = new Bundle();
                            b.putString("operation", str);
                            intent.putExtras(b);
                            intent.setIdentifier("operation");
                            sendBroadcast(intent);
                        }
                    }
//                    else {
//                        if (topic.endsWith("readings")) {
//                            Bundle b = gson.fromJson(str, Bundle.class);
//                            Readings readings = new Readings(b);
//                            readings.broadcastReadings(MonitorApplication.getAppContext(), "MQTT", Constants.CA_FARRELLTONSOLAR_CLASSIC_READINGS_SLAVE);
//                        }
//                        else if (topic.endsWith("LWT")) {
//                            if (str.compareTo("Offline") == 0) {
//                                clearReadings(Constants.CA_FARRELLTONSOLAR_CLASSIC_READINGS_SLAVE);
//                                chargeControllers.setReachable(deviceName, false);
//                            } else {
//                                chargeControllers.setReachable(deviceName, true);
//                                WakeMQTT("wake");
//                            }
//                        }
//                    }
                } catch (Exception e) {
                    Log.w(getClass().getName(), "MQTT deserialize Exception " + topic);
                    e.printStackTrace();
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
                Log.d(getClass().getName(), "MQTT Subscriber deliveryComplete ");
            }
        });
    }

    private void UnSubscribe() throws MqttException {
        if (mqttClient != null && mqttClient.isConnected()) {
            try {
                UnSubscribeTo(commandTopic);
                UnSubscribeTo(statusTopic);
            } catch (Exception e) {
                Log.w(getClass().getName(), e.getMessage());
                e.printStackTrace();
            }
        }
        if (mqttWakeTimer != null) {
            mqttWakeTimer.cancel();
            mqttWakeTimer.purge();
            Log.d(getClass().getName(), "mqttWakeTimer.purge");
        }
    }

    private void UnSubscribeTo(String topic) throws MqttException {
        IMqttToken token = mqttClient.unsubscribe(topic);
        token.setActionCallback(new IMqttActionListener() {
            @Override
            public void onSuccess(IMqttToken iMqttToken) {
                Log.d(getClass().getName(), " UnSubscribe Successful ");
            }

            @Override
            public void onFailure(IMqttToken iMqttToken, Throwable throwable) {
                Log.w(getClass().getName(), " UnSubscribe Failed " + iMqttToken.getException().getMessage());
            }
        });
    }

}
