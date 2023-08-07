// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.privacy.ppn.krypton;

import com.google.android.libraries.privacy.ppn.IpGeoLevel;
import com.google.android.libraries.privacy.ppn.internal.KryptonConfig;
import com.google.android.libraries.privacy.ppn.internal.KryptonTelemetry;
import com.google.android.libraries.privacy.ppn.internal.NetworkInfo;
import org.json.JSONObject;

/**
 * Interface to Krypton native library.
 *
 * <p>To start handling VPN traffic, call start(). When it's time to stop the VPN service, callers
 * must call stop() to close any open file descriptors and free memory used by Krypton. All
 * lifecycle events for Krypton will be reported by the provided KryptonListener.
 */
public interface Krypton {
  /**
   * Starts Krypton running in the background.
   *
   * <p>Implemented in the native library |Krypton::Start|.
   */
  void start(KryptonConfig config) throws KryptonException;

  /**
   * Stops the Krypton service, closing any open connections.
   *
   * <p>Implemented in the native library |Krypton::Stop|
   */
  void stop() throws KryptonException;

  /** Snoozes the Krypton service, closing open connections. */
  void snooze(long snoozeDurationMs) throws KryptonException;

  /** Extends how long Krypton service is snoozed for. */
  void extendSnooze(long extendSnoozeDurationMs) throws KryptonException;

  /**
   * Resumes Krypton service.
   *
   * <p>Implemented in the native library |Krypton::Resume|
   */
  void resume() throws KryptonException;

  /**
   * Switches the outbound network of the device.
   *
   * <p>Implemented by the native library in |Krypton::setNetwork|
   *
   * <p>Caller should reserve (bind and protect) the network_fd socket before calling this method.
   * Caller also relinquishes the ownership of the FD except when an exception is thrown.
   */
  void setNetwork(NetworkInfo networkInfo) throws KryptonException;

  /**
   * Indicates that no networks are available. The behavior of Krypton could be Fail Open or Fail
   * Closed.
   *
   * <p>Implemented by the native library in |Krypton::SetNoNetworkAvailable|
   */
  void setNoNetworkAvailable() throws KryptonException;

  /** Update the state of the Safe Disconnect feature in Krypton. */
  void setSafeDisconnectEnabled(boolean enable) throws KryptonException;

  /** Returns whether Safe Disconnect is enabled in Krypton. */
  boolean isSafeDisconnectEnabled() throws KryptonException;

  /** Update the level of IP geo used by PPN. Will cause a reconnect. */
  void setIpGeoLevel(IpGeoLevel level) throws KryptonException;

  /** Gets the IP geo level currently in use. */
  IpGeoLevel getIpGeoLevel() throws KryptonException;

  /** Puts Krypton in a horrible wedged state, for testing app bypass, etc. */
  void setSimulatedNetworkFailure(boolean simulatedNetworkFailure) throws KryptonException;

  /** Collects telemetry data from Krypton, and resets it. */
  KryptonTelemetry collectTelemetry() throws KryptonException;

  /** Collects debug info from Krypton. */
  JSONObject getDebugJson() throws KryptonException;

  /** Disables the keepalive being used in the native library. */
  void disableKryptonKeepalive() throws KryptonException;

  /**
   * Creates a new tunnel to be used by Krypton, if one already exists. If there is no tunnel
   * already this will not create one.
   *
   * <p>When settings are updated in VpnManager the updates are not applied to the existing tunnel.
   * Applying these settings requires the creation of a new tunnel. The settings in VpnManager that
   * require a tunnel update to take effect are the list of disallow applications and allowBypass.
   * After these settings are changed in VpnManager this function can be called to create a tunnel
   * with the new values.
   */
  void forceTunnelUpdate() throws KryptonException;
}
