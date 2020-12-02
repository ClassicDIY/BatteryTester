/*
 *  Created by ClassicDIY on 02/12/20 9:36 AM
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

package ca.classicdiy.battery.ui.dashboard;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

import ca.classicdiy.battery.Constants;
import ca.classicdiy.battery.R;

public class InfoListAdapter extends ArrayAdapter<Bundle> {

    private boolean isReceiverRegistered = false;

    ArrayList<Bundle> cells = new ArrayList<Bundle>();
    public InfoListAdapter(Context context) {
        super(context, R.layout.battery_item);
        for (int i = 0; i < 16; i++) {
            cells.add(new Bundle());
        }
        this.addAll(cells);
        this.notifyDataSetChanged();
    }


    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view;

        LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (convertView == null) {
            view = inflater.inflate(R.layout.battery_item, parent, false);
        } else {
            view = convertView;
        }

        Bundle item = getItem(position);
        if (item != null) {
            TextView cellNumber = (TextView) view.findViewById(R.id.cellNumber);
            cellNumber.setText(String.format("%d", position+1));
            TextView stage = (TextView) view.findViewById(R.id.stage);
            stage.setText(cells.get(position).getString("state", ""));
            TextView voltage = (TextView) view.findViewById(R.id.voltage);
            voltage.setText(String.format("%d mV", cells.get(position).getInt("voltage", 0)));
            TextView current = (TextView) view.findViewById(R.id.current);
            current.setText(String.format("%d mA", cells.get(position).getInt("current", 0)));
            TextView temperature = (TextView) view.findViewById(R.id.temperature);
            temperature.setText(String.format("%2.1f °C", cells.get(position).getInt("temperature")/10.0));
            TextView resistance = (TextView) view.findViewById(R.id.resistance);
            resistance.setText(String.format("%d mΩ", cells.get(position).getInt("resistance")));
            TextView energy = (TextView) view.findViewById(R.id.energy);
            energy.setText(String.format("%d mAh", cells.get(position).getInt("energy", 0)));
        }
        return view;
    }

     public void onStop() {
        if (isReceiverRegistered) {
            try {
                getContext().unregisterReceiver(mReadingsReceiver);
            } catch (IllegalArgumentException e) {
                // Do nothing
            }
            isReceiverRegistered = false;
        }
    }

    public void onStart() {
        if (!isReceiverRegistered) {
            getContext().registerReceiver(mReadingsReceiver, new IntentFilter(Constants.STATUS_SUBTOPIC));
            isReceiverRegistered = true;
        }
    }

    public void update(Bundle b) {
        int pos = b.getInt("index");
        cells.set(pos, b); // replace bundle
        this.notifyDataSetChanged();
    }

    protected BroadcastReceiver mReadingsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {

                if ("monitor".equals(intent.getIdentifier())) {
                    Bundle b = intent.getBundleExtra("monitor");
                    update(b);
                    Log.d(getClass().getName(), String.format("monitor - Battery(%d) %s: %d mV %d mA %2.1f °C",  b.getInt("index"), b.getString("state"), b.getInt("voltage"), b.getInt("current"), b.getInt("temperature")/10.0) );
                }
                if ("result".equals(intent.getIdentifier())) {
                    Bundle b = intent.getBundleExtra("result");
                    Log.d(getClass().getName(), String.format("result - Battery(%d) %s: %d mV %d mA %2.1f °C",  b.getInt("index"), b.getString("state"), b.getInt("voltage"), b.getInt("current"), b.getInt("temperature")/10.0) );

                }

        }
    };

}
