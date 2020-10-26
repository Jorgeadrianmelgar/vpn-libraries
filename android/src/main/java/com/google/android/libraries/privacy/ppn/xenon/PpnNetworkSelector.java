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

package com.google.android.libraries.privacy.ppn.xenon;

import com.google.android.libraries.privacy.ppn.internal.ConnectionStatus.ConnectionQuality;
import java.util.List;

/**
 * PpnNetworkSelector handles choosing the best network to use from the provided list of
 * PpnNetworks.
 */
public interface PpnNetworkSelector {

  /**
   * Returns the best PpnNetwork to use from the passed in PpnNetwork List. If there is no networks
   * in the passed in list, this method will return Null.
   */
  PpnNetwork getBestNetwork(List<PpnNetwork> ppnNetworkList);

  /**
   * Determines and returns the ConnectionQuality for the PpnNetwork. If the rssi arg is not
   * populated (e.g. value of 0), we will get it from the PpnNetwork as appropriate depending on the
   * NetworkType.
   *
   * <p>Note: the rssi should be set when it is available from the NetworkCapabilities depending on
   * the Android version of the device.
   */
  ConnectionQuality getConnectionQuality(PpnNetwork ppnNetwork, int rssi);
}
