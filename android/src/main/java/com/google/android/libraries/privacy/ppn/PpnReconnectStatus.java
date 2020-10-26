// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the );
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an  BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.privacy.ppn;

import java.time.Duration;

/**
 * Status for the time until PPN's next reconnection attempt. Internally, we store this time as a
 * Java duration.
 */
public class PpnReconnectStatus {
  private final Duration timeToReconnect;

  /** Construct a PpnReconnectStatus with a Java duration. */
  public PpnReconnectStatus(Duration timeToReconnect) {
    this.timeToReconnect = timeToReconnect;
  }

  /** Returns the time until Krypton's next reconnection attempt, as a Java duration. */
  public Duration getTimeToReconnect() {
    return this.timeToReconnect;
  }

  @Override
  public String toString() {
    return timeToReconnect.toString();
  }
}
