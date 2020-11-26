/*
 *  Created by ClassicDIY on 25/11/20 6:57 AM
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

public enum State {
    Unspecified,
    Initialize,
    Standby,
    NoBatteryFound,
    Monitor,
    Stabilize,
    InternalResistance,
    FullCharge,
    Discharge,
    StorageCharge,
    ThermalShutdown,
    CycleConplete,
    Complete;

    private static State[] values = null;

    public static State fromInt(int i) {
        if (State.values == null) {
            State.values = State.values();
        }
        return State.values[i];
    }
}

