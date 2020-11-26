/*
 *  Created by ClassicDIY on 25/11/20 6:54 AM
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

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

public class Readings {
    final Object lock = new Object();

    public Readings() {
        readings = new Bundle();
    }

    public Readings(Bundle init) {
        readings = init;
    }

    Bundle readings;

    public Bundle getReadings() {
        return readings;
    }

    public Float getFloat(ElementId name) {
        synchronized (lock) {
            return readings.getFloat(name.name(), 0);
        }
    }

    public int getInt(ElementId name) {
        synchronized (lock) {
            return readings.getInt(name.name(), 0);
        }
    }

    public String GetString(ElementId name) {
        synchronized (lock) {
            return readings.getString(name.name());
        }
    }


    public Boolean getBoolean(ElementId name) {
        synchronized (lock) {
            return readings.getBoolean(name.name(), false);
        }
    }

    public void set(ElementId name, Float value) {
        synchronized (lock) {
            readings.putFloat(name.name(), value);
        }
    }

    public void set(ElementId name, int value) {
        synchronized (lock) {
            readings.putInt(name.name(), value);
        }
    }

    public void set(ElementId name, String value) {
        synchronized (lock) {
            readings.putString(name.name(), value);
        }
    }

    public void set(ElementId name, Boolean value) {
        synchronized (lock) {
            readings.putBoolean(name.name(), value);
        }
    }

    public void broadcastReadings(Context context, String uniqueId, String action) {
        if (!readings.isEmpty()) {
            Intent intent = new Intent(action);
            intent.putExtra("readings", readings);
            intent.putExtra("uniqueId", uniqueId);
            context.getApplicationContext().sendBroadcast(intent);
        }
    }
}
