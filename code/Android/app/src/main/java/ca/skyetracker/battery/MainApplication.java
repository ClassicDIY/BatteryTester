package ca.skyetracker.battery;

import android.app.Application;
import android.content.Context;
import android.content.Intent;

/**
 * Created by Me on 10/21/2017.
 */

public class MainApplication extends Application {
    static Context context;
    public void onCreate() {
        super.onCreate();
        context = this.getBaseContext();
        Intent bluetoothInitIntent = new Intent("ca.skyetracker.battery.Connect", null, context, BlueTooth.class);
        this.startService(bluetoothInitIntent);
    }

}
