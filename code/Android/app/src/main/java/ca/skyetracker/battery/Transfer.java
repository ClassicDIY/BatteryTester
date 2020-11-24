package ca.skyetracker.battery;

import java.io.Serializable;

/**
 * Created by Me on 10/21/2017.
 */

/*
	_BlueTooth.print("Cell|{");
	_BlueTooth.print("\"sN\":");
	_BlueTooth.print(index);
	_BlueTooth.print(",\"sS\":");
	_BlueTooth.print(_dischargers[index].State());
	_BlueTooth.print(",\"sR\":");
	_BlueTooth.print(_dischargers[index].InternalResistance());
	_BlueTooth.print(",\"sV\":");
	_BlueTooth.print(_dischargers[index].BatteryVolt());
	_BlueTooth.print(",\"sN\":");
	_BlueTooth.print(_dischargers[index].BatteryCurrent());
	_BlueTooth.print(",\"sI\":");
	_BlueTooth.print(_dischargers[index].Capacity());
	_BlueTooth.print(",\"sE\":");
	_BlueTooth.print(_dischargers[index].Temperature());
	_BlueTooth.print(",\"sQ\":");
	_BlueTooth.print(_dischargers[index].ElapsedTime());
	_BlueTooth.println("}");

 */
public class Transfer implements Serializable {
    public int sN;
    public int sS;
    public int sR;
    public float sV;
    public int sI;
    public int sQ;
    public float sT;
    public int sE;

    public Transfer(String data ) {
        String line = data.substring(1, data.length() -1);
        String[] tokens = line.split(",");
        if (tokens.length == 8) {
            try {
                sN = Integer.parseInt(tokens[0], 16);
                sS = Integer.parseInt(tokens[1], 16);
                sR = Integer.parseInt(tokens[2], 16);
                sV = Integer.parseInt(tokens[3], 16)/1000f;
                sI = Integer.parseInt(tokens[4], 16);
                sQ = Integer.parseInt(tokens[5], 16);
                sT = Integer.parseInt(tokens[6], 16)/10f;
                sE = Integer.parseInt(tokens[7], 16);
            }
            catch (Exception ex){
                String err = line;
            }
        }
    }
};