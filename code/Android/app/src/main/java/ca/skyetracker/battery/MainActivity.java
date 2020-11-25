package ca.skyetracker.battery;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.Bundle;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import androidx.viewpager.widget.ViewPager;

import android.widget.Toast;
import java.util.ArrayList;

import static ca.skyetracker.battery.State.Discharge;

public class MainActivity extends Activity {
    private TabStripAdapter tabStripAdapter;
    private SlidingTabLayout stl;
    private ViewPager viewPager;
    boolean isReceiverRegistered = false;
    int last_sQ = -1;
    public ArrayList<RecordEntry> records[] = new ArrayList[4];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        stl = (SlidingTabLayout) findViewById(R.id.sliding_tabs);
        stl.setDividerColors(Color.RED);
        stl.setSelectedIndicatorColors(Color.GREEN, Color.MAGENTA, Color.YELLOW, Color.WHITE, Color.CYAN);
        viewPager = (ViewPager) findViewById(R.id.pager);
        setupActionBar();
        records[0] = new ArrayList<RecordEntry>();
        records[1] = new ArrayList<RecordEntry>();
        records[2] = new ArrayList<RecordEntry>();
        records[3] = new ArrayList<RecordEntry>();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelableArrayList("record0", records[0]);
        outState.putParcelableArrayList("record1", records[1]);
        outState.putParcelableArrayList("record2", records[2]);
        outState.putParcelableArrayList("record3", records[3]);

    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        if (savedInstanceState != null) {
            if (savedInstanceState.containsKey("record0")) {
                records[0] = savedInstanceState.getParcelableArrayList("record0");
            } else {
                records[0] = new ArrayList<RecordEntry>();
            }
            if (savedInstanceState.containsKey("record1")) {
                records[1] = savedInstanceState.getParcelableArrayList("record1");
            } else {
                records[1] = new ArrayList<RecordEntry>();
            }
            if (savedInstanceState.containsKey("record2")) {
                records[2] = savedInstanceState.getParcelableArrayList("record2");
            } else {
                records[2] = new ArrayList<RecordEntry>();
            }
            if (savedInstanceState.containsKey("record3")) {
                records[3] = savedInstanceState.getParcelableArrayList("record3");
            } else {
                records[3] = new ArrayList<RecordEntry>();
            }
        }
    }

    private void setupActionBar() {
        tabStripAdapter = new TabStripAdapter(getFragmentManager(), this, viewPager, stl, new ViewPager.OnPageChangeListener() {

            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {

            }

            // This method will be invoked when a new page becomes selected.
            @Override
            public void onPageSelected(int position) {
            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });
        Bundle b = new Bundle();
        b.putInt("Index", 0);
        tabStripAdapter.addTab(R.string.One, InfoTab.class, b);
        b = new Bundle();
        b.putInt("Index", 1);
        tabStripAdapter.addTab(R.string.Two, InfoTab.class, b);
        b = new Bundle();
        b.putInt("Index", 2);
        tabStripAdapter.addTab(R.string.Three, InfoTab.class, b);
        b = new Bundle();
        b.putInt("Index", 3);
        tabStripAdapter.addTab(R.string.Four, InfoTab.class, b);
        tabStripAdapter.notifyTabsChanged();

    }


    private void errorExit(String title, String message) {
        Toast.makeText(getBaseContext(), title + " - " + message, Toast.LENGTH_LONG).show();
        finish();
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (!isReceiverRegistered) {
            LocalBroadcastManager.getInstance(this).registerReceiver(mReadingsReceiver, new IntentFilter("ca.skyetracker.battery.cell"));
            isReceiverRegistered = true;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isReceiverRegistered) {
            try {
                LocalBroadcastManager.getInstance(this).unregisterReceiver(mReadingsReceiver);
            } catch (IllegalArgumentException e) {
                // Do nothing
            }
            isReceiverRegistered = false;
        }
    }

    protected BroadcastReceiver mReadingsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            try {
                Transfer cellTransfer = (Transfer)intent.getSerializableExtra("cell");
                if (State.fromInt(cellTransfer.sS) == Discharge && last_sQ != cellTransfer.sQ) {
                    last_sQ = cellTransfer.sQ;
                    int index = cellTransfer.sN;
                    ArrayList<RecordEntry> record = records[index];
                    if (record.size() > 10000) {
                        record.remove(0);
                    }
                    record.add(new RecordEntry(cellTransfer.sV, cellTransfer.sQ));
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    };
}
