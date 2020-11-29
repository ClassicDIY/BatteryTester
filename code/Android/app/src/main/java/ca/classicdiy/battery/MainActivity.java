/*
 *  Created by ClassicDIY on 26/11/20 10:11 AM
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.navigation.NavigationView;

import java.util.HashMap;
import java.util.Map;

import static ca.classicdiy.battery.Constants.getEnumFromString;

public class MainActivity extends AppCompatActivity {

    private NavigationView navigationView;
    public static Operation currentOperation = Operation.Monitor;
    private DrawerLayout drawer;
    private Toolbar toolbar;
    private boolean isReceiverRegistered = false;
    private Map<Operation, Integer> operationMap = new HashMap<Operation, Integer>(){{
        put(Operation.Monitor, R.id.nav_home);
        put(Operation.Cycle, R.id.nav_cycle);
        put(Operation.Charge, R.id.nav_charge);
        put(Operation.TestAndStore, R.id.nav_TestAndStore);
        put(Operation.TestAndCharge, R.id.nav_TestAndCharge);
        put(Operation.Storage, R.id.nav_Storage);
        put(Operation.InternalResistance, R.id.nav_InternalResistance);
        put(Operation.Discharge, R.id.nav_Discharge);
    }};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        drawer = findViewById(R.id.drawer_layout);
        navigationView = findViewById(R.id.mode_view);
        ActionBarDrawerToggle actionBarDrawerToggle = new ActionBarDrawerToggle(this, drawer, toolbar, R.string.openDrawer, R.string.closeDrawer) {

            @Override
            public void onDrawerClosed(View drawerView) {
                super.onDrawerClosed(drawerView);
            }

            @Override
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);
            }
        };
        drawer.addDrawerListener(actionBarDrawerToggle);
        //calling sync state is necessary or else your hamburger icon wont show up
        actionBarDrawerToggle.syncState();
        setUpModeView();
        BottomNavigationView navView = findViewById(R.id.nav_view);
        AppBarConfiguration appBarConfiguration = new AppBarConfiguration.Builder(
                R.id.navigation_home, R.id.navigation_dashboard, R.id.navigation_notifications)
                .build();
        NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment);
        NavigationUI.setupActionBarWithNavController(this, navController, appBarConfiguration);
        NavigationUI.setupWithNavController(navView, navController);
        if (!isReceiverRegistered) {
            this.registerReceiver(cmndReceiver, new IntentFilter(Constants.COMMAND_SUBTOPIC));
            isReceiverRegistered = true;
        }
    }

    @Override
    protected void onDestroy()
    {
        if (!isReceiverRegistered) {
            this.unregisterReceiver(cmndReceiver);
            isReceiverRegistered = false;
        }
        super.onDestroy();
    }

    protected BroadcastReceiver cmndReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String operation = intent.getStringExtra("operation");
            Operation newOperation = getEnumFromString(Operation.class, operation);
            if (currentOperation != newOperation) { // operation changed, set the action bar title and check menuItem
                currentOperation = newOperation;
                Log.d(getClass().getName(), operation);
                Integer id = operationMap.getOrDefault(currentOperation, R.id.nav_home);
                Menu m = navigationView.getMenu();

                MenuItem menuItem = m.findItem(id);
                if (menuItem != null) {
                    menuItem.setChecked(true);
                    toolbar.setTitle(menuItem.getTitle());
                }
            }
        }
    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. (settings)
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            startActivityForResult(new Intent(this, Settings.class), 5);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void setUpModeView() {
        //Setting Navigation View Item Selected Listener to handle the item click of the navigation menu
        navigationView.setNavigationItemSelectedListener(new NavigationView.OnNavigationItemSelectedListener() {

            // This method will trigger on item Click of navigation menu
            @Override
            public boolean onNavigationItemSelected(MenuItem menuItem) {

                //Check to see which item was being clicked and perform appropriate action
                switch (menuItem.getItemId()) {
                    //Replacing the main content with ContentFragment Which is our Inbox View;
                    case R.id.home:
                        currentOperation = Operation.Monitor;
                        break;
                    case R.id.nav_cycle:
                        currentOperation = Operation.Cycle;
                        break;
                    case R.id.nav_charge:
                        currentOperation = Operation.Charge;
                        break;
                    case R.id.nav_TestAndStore:
                        currentOperation = Operation.TestAndStore;
                        break;
                    case R.id.nav_TestAndCharge:
                        currentOperation = Operation.TestAndCharge;
                        break;
                    case R.id.nav_Storage:
                        currentOperation = Operation.Storage;
                        break;
                    case R.id.nav_InternalResistance:
                        currentOperation = Operation.InternalResistance;
                        break;
                    case R.id.nav_Discharge:
                        currentOperation = Operation.Discharge;
                        break;

                    case R.id.nav_about_us:
                        // launch new intent instead of loading fragment
//                    startActivity(new Intent(MainActivity.this, AboutUsActivity.class));
//                    drawer.closeDrawers();
                        return true;
                    case R.id.nav_settings:
                        // launch new intent instead of loading fragment
//                    startActivity(new Intent(MainActivity.this, PrivacyPolicyActivity.class));
//                    drawer.closeDrawers();
                        return true;
                    default:
                        currentOperation = Operation.Monitor;
                }
                toolbar.setTitle(menuItem.getTitle());
                menuItem.setChecked(true);
                drawer.closeDrawers();
                Intent commandIntent = new Intent().setAction(Constants.COMMAND_SUBTOPIC);
                commandIntent.putExtra("operation", currentOperation.name());
                commandIntent.setIdentifier("publish");
                sendBroadcast(commandIntent);
                return true;
            }
        });
    }
}

