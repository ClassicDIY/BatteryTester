package ca.skyetracker.battery;

/**
 * Created by Me on 10/22/2017.
 */

import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.mikephil.charting.components.Legend;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.interfaces.datasets.ILineDataSet;
import com.github.mikephil.charting.utils.ColorTemplate;

import static ca.skyetracker.battery.State.Discharge;

/**
 * Created by Me on 9/30/2015.
 */
public class InfoTab extends Fragment {
    private CustomLineChart mChart;
    TextView textState, textEnergy, textTime, textVoltage, textCurrent, textResistance, textTemperature;
    Context context;
    int index;
    private int last_sQ;

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        context = container.getContext();
        Bundle b = getArguments();
        index = b.getInt("Index");
        View rootView = inflater.inflate(R.layout.info, container, false);
        textVoltage = (TextView) rootView.findViewById(R.id.textVoltage);
        textCurrent = (TextView) rootView.findViewById(R.id.textCurrent);
        textResistance = (TextView) rootView.findViewById(R.id.textResistance);
        textTemperature = (TextView) rootView.findViewById(R.id.textTemperature);
        textState = (TextView) rootView.findViewById(R.id.textState);
        textEnergy = (TextView) rootView.findViewById(R.id.textEnergy);
        textTime = (TextView) rootView.findViewById(R.id.textTime);
        LocalBroadcastManager.getInstance(context).registerReceiver(mReadingsReceiver, new IntentFilter("ca.skyetracker.battery.cell"));
        return rootView;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initializeReadings(view, savedInstanceState);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        LocalBroadcastManager.getInstance(context).unregisterReceiver(mReadingsReceiver);
        this.setRetainInstance(true);
    }

//    enum State {
//        Standby,
//        NoBatteryFound,
//        MeasuringResistance,
//        InitialCharge,
//        Discharge,
//        FinalCharge,
//        ThermalShutdown,
//        Complete
//    };


    protected BroadcastReceiver mReadingsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                Transfer cellTransfer = (Transfer)intent.getSerializableExtra("cell");
                if (cellTransfer.sN == index) {
                    switch (State.fromInt(cellTransfer.sS)) {
                        case Initialized:
                            textState.setText("Initialized");
                            break;
                        case Standby:
                            textState.setText("Standby");
                            break;
                        case NoBatteryFound:
                            textState.setText("No Battery");
                            break;
                        case MeasuringResistance:
                            textState.setText("Measuring Resistance");
                            break;
                        case InitialCharge:
                            textState.setText("Inital Charge");
                            break;
                        case Discharge:
                            textState.setText("Discharging");
                            break;
                        case FinalCharge:
                            textState.setText("Final Charge");
                            break;
                        case ThermalShutdown:
                            textState.setText("Thermal Shutdown");
                            break;
                        case Complete:
                            textState.setText("Complete");
                            break;
                    }
                    textVoltage.setText(String.format("%.2fV", cellTransfer.sV));
                    textCurrent.setText(String.format("%dmA", cellTransfer.sI));
                    textResistance.setText(String.format("%dmΩ", cellTransfer.sR));
                    textTemperature.setText(String.format("%.1f C", cellTransfer.sT));
                    textEnergy.setText(String.format("%dmAh", cellTransfer.sQ));
                    int minutes = cellTransfer.sE / 60;
                    int seconds = cellTransfer.sE % 60;
                    textTime.setText(String.format("%s:%s m:s", minutes, seconds));
                    if (State.fromInt(cellTransfer.sS) == Discharge && last_sQ != cellTransfer.sQ) {
                        last_sQ = cellTransfer.sQ;
                        addEntry(cellTransfer.sV, cellTransfer.sQ);
                    }
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    };

    private void addEntry(float volts, int energy) {
        try {

            LineData data = mChart.getData();
            if (data != null) {
                ILineDataSet set = data.getDataSetByIndex(0);
                // add a new x-value first
                data.addXValue(Integer.toString(energy));
                data.addEntry(new Entry(volts, set.getEntryCount(), RegisterName.BatVoltage), 0);
//                data.addEntry(new Entry(currentInAmps, set.getEntryCount(), RegisterName.BatCurrent), 1);

                // let the chart know it's data has changed
                mChart.notifyDataSetChanged();
                // move to the latest entry
                mChart.moveViewToX(data.getXValCount());
            }

        } catch (Exception all) {
            Log.w(getClass().getName(), String.format("setReadings Exception ex: %s", all));
        }
    }

    public void initializeReadings(View view, Bundle savedInstanceState) {
        mChart = (CustomLineChart) view.findViewById(R.id.chart1);
        mChart.setDescription("");
        mChart.setTouchEnabled(true);
        mChart.setDragEnabled(true);
        mChart.setScaleEnabled(true);
        mChart.setDrawGridBackground(false);
        mChart.setPinchZoom(true);
        mChart.setBackgroundColor(Color.TRANSPARENT);
        LineData data = new LineData();
        data.setValueTextColor(Color.GREEN);
        // add empty data
        mChart.setData(data);
        // get the legend (only possible after setting data)
        Legend l = mChart.getLegend();
        l.setForm(Legend.LegendForm.LINE);
        l.setTextColor(Color.WHITE);
        XAxis xl = mChart.getXAxis();
        xl.setTextColor(Color.BLACK);
        xl.setDrawGridLines(false);
        xl.setAvoidFirstLastClipping(true);
        xl.setSpaceBetweenLabels(5);
        xl.setEnabled(true);

        YAxis leftAxis = mChart.getAxisLeft();
        leftAxis.setTextColor(Color.DKGRAY);
        leftAxis.setDrawGridLines(true);
        leftAxis.setTextSize(14f);
        leftAxis.setLabelCount(10, false);
        leftAxis.setAxisMaxValue(4.5f);
        leftAxis.setAxisMinValue(2.5f);
        YAxis rightAxis = mChart.getAxisRight();
        rightAxis.setEnabled(false);

        ILineDataSet set = data.getDataSetByIndex(0);
        if (set == null) {
            data.addDataSet(createVoltsSet());
        }
        MainActivity ma = (MainActivity)getActivity();
        for (RecordEntry r : ma.records[index]) {
            addEntry(r.volt, r.energy);
        }
//        set = data.getDataSetByIndex(1);
//        if (set == null) {
//            data.addDataSet(createCurrentSet());
//        }
    }

    private LineDataSet createVoltsSet() {
        LineDataSet set = new LineDataSet(null, "Volts");
        set.setAxisDependency(YAxis.AxisDependency.LEFT);
        set.setColor(Color.rgb(238,0,35));
        set.setLineWidth(2f);
        set.setDrawCircles(false);
        set.setFillAlpha(65);
        set.setFillColor(ColorTemplate.getHoloBlue());
        set.setHighLightColor(Color.rgb(244, 117, 117));
        set.setValueTextColor(Color.BLACK);
        set.setValueTextSize(14f);
        set.setDrawValues(false);
        set.setHighlightEnabled(true);
        return set;
    }

    private LineDataSet createCurrentSet() {
        LineDataSet set = new LineDataSet(null, "Current");
        set.setAxisDependency(YAxis.AxisDependency.RIGHT);
        set.setColor(Color.rgb(0, 0, 0));
        set.setLineWidth(2f);
        set.setDrawCircles(false);
        set.setFillAlpha(65);
        set.setFillColor(ColorTemplate.getHoloBlue());
        set.setHighLightColor(Color.rgb(244, 117, 117));
        set.setValueTextColor(Color.BLACK);
        set.setValueTextSize(14f);
        set.setDrawValues(false);
        set.setHighlightEnabled(true);
        return set;
    }

    public enum RegisterName {
        BatVoltage,
        BatCurrent,

    }
}
