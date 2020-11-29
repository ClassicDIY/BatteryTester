/*
 *  Created by ClassicDIY on 26/11/20 9:55 AM
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

import java.util.UUID;

public class Constants {
    public static final String TAG = "BatteryTester";

    // SPP UUID service
    public static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    public static final String CA_BATTERY_TOAST = "ca.battery.Toast";

    public static final String COMMAND_SUBTOPIC = "ca.battery.cmnd"; // command subtopic
    public static final String STATUS_SUBTOPIC = "ca.battery.stat"; // status subtopic
    public static final String TELEMETRY_SUBTOPIC = "ca.battery.tele"; // telemetry subtopic


    public static <T extends Enum<T>> T getEnumFromString(Class<T> c, String string) {
        if( c != null && string != null ) {
            try {
                return Enum.valueOf(c, string.trim());
            } catch(IllegalArgumentException ex) {
            }
        }
        return null;
    }
}
