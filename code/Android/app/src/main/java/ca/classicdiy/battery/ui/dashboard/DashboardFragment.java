package ca.classicdiy.battery.ui.dashboard;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.ListFragment;

import ca.classicdiy.battery.Constants;

public class DashboardFragment extends ListFragment {
    private boolean isReceiverRegistered = false;
    private  InfoListAdapter adapter;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {


        adapter = new InfoListAdapter(inflater.getContext());
        setListAdapter(adapter);
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onStop() {
        super.onStop();
        adapter.onStop();
    }

    @Override
    public void onStart() {
        super.onStart();
        adapter.onStart();
    }
}