package ca.skyetracker.battery;

public enum State {
    Initialized,
    Standby,
    NoBatteryFound,
    MeasuringResistance,
    InitialCharge,
    Discharge,
    FinalCharge,
    ThermalShutdown,
    Complete;

    private static State[] values = null;

    public static State fromInt(int i) {
        if (State.values == null) {
            State.values = State.values();
        }
        return State.values[i];
    }
}

