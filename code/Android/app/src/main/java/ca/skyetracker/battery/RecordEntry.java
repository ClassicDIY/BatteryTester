package ca.skyetracker.battery;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by Me on 10/24/2017.
 */

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