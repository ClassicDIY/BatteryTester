package ca.skyetracker.battery;

import java.util.UUID;

/**
 * Created by Me on 10/21/2017.
 */

public class Constants {
    public static final String TAG = "BlueTooth";

    // SPP UUID service
    public static final UUID MY_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    // MAC-address of Bluetooth module (you must edit this line)
    public static String address = "20:13:06:14:34:87";
//    public static String address = "98:d3:31:b3:68:dd";
}
