/*
 *  Created by ClassicDIY on 25/11/20 6:56 AM
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

import android.os.Parcel;
import android.os.Parcelable;

public class RecordEntry implements Parcelable {

    public RecordEntry(float v, int e) {
        volt = v;
        energy = e;
    }

    private RecordEntry(Parcel in) {
        volt = in.readFloat();
        energy = in.readInt();
    }

    public float volt;
    public int energy;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeFloat(volt);
        dest.writeInt(energy);
    }

    public static final Parcelable.Creator<RecordEntry> CREATOR = new Parcelable.Creator<RecordEntry>() {
        public RecordEntry createFromParcel(Parcel in) {
            return new RecordEntry(in);
        }

        public RecordEntry[] newArray(int size) {
            return new RecordEntry[size];
        }
    };
}