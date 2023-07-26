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

package com.google.android.libraries.privacy.ppn.internal.http;

import android.net.Network;
import javax.net.SocketFactory;

/** A factory that creates SocketFactory instances that are bound to specific networks. */
public interface BoundSocketFactoryFactory {
  /**
   * Returns a SocketFactory that will bind the socket to the current network, as defined by the
   * implementation.
   */
  SocketFactory withCurrentNetwork();

  /** Returns a SocketFactory that will bind the socket to the given network. */
  SocketFactory withNetwork(Network network);
}
