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
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.ListFragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import ca.classicdiy.battery.Constants;
import ca.classicdiy.battery.InfoListAdapter;
import ca.classicdiy.battery.R;

public class DashboardFragment extends ListFragment {
    private boolean isReceiverRegistered = false;


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        Pair[] data = new Pair[10];
        InfoListAdapter adapter = new InfoListAdapter(inflater.getContext(), data);
        setListAdapter(adapter);
        return super.onCreateView(inflater, container, savedInstanceState);
    }

//    @Override
//    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
//        super.onActivityCreated(savedInstanceState);
//        mViewModel = new ViewModelProvider(this).get(MainViewModel.class);
//        // TODO: Use the ViewModel
//    }

    @Override
    public void onStop() {
        super.onStop();
        if (isReceiverRegistered) {
            try {
                this.getActivity().unregisterReceiver(mReadingsReceiver);
            } catch (IllegalArgumentException e) {
                // Do nothing
            }
            isReceiverRegistered = false;
        }
        Log.d(getClass().getName(), "onStop");
    }

    @Override
    public void onStart() {
        super.onStart();
        if (!isReceiverRegistered) {
            this.getActivity().registerReceiver(mReadingsReceiver, new IntentFilter(Constants.STATUS_SUBTOPIC));
            isReceiverRegistered = true;
        }
    }

    protected BroadcastReceiver mReadingsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (isVisible()) {
                if ("monitor".equals(intent.getIdentifier())) {
                    Bundle b = intent.getBundleExtra("monitor");
                    Log.d(getClass().getName(), String.format("monitor - Battery(%d) %s: %d mV %d mA %2.1f °C",  b.getInt("index"), b.getString("state"), b.getInt("voltage"), b.getInt("current"), b.getInt("temperature")/10.0) );
                }
                if ("result".equals(intent.getIdentifier())) {
                    Bundle b = intent.getBundleExtra("result");
                    Log.d(getClass().getName(), String.format("result - Battery(%d) %s: %d mV %d mA %2.1f °C",  b.getInt("index"), b.getString("state"), b.getInt("voltage"), b.getInt("current"), b.getInt("temperature")/10.0) );

                }
            }
        }
    };
}