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

package com.google.android.libraries.privacy.ppn.internal.service;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import android.net.VpnService;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.RobolectricTestRunner;

/** Unit test for {@link RouteManager}. */
@RunWith(RobolectricTestRunner.class)
public class RouteManagerTest {
  @Rule public final MockitoRule mocks = MockitoJUnit.rule();

  @Mock private VpnService.Builder mockBuilder;

  @Test
  public void addIpv4Routes_addsSingleRoute() {
    RouteManager.addIpv4Routes(mockBuilder, /* excludeLocalAddresses= */ false);

    verify(mockBuilder).addRoute(anyString(), anyInt());
    verifyNoMoreInteractions(mockBuilder);
  }

  @Test
  public void addIpv4Routes_excludeLocalAddressesAddsMultipleRoutes() {
    RouteManager.addIpv4Routes(mockBuilder, /* excludeLocalAddresses= */ true);

    verify(mockBuilder, atLeast(2)).addRoute(anyString(), anyInt());
    verifyNoMoreInteractions(mockBuilder);
  }

  @Test
  public void addIpv6Routes_addsSingleRoute() {
    RouteManager.addIpv6Routes(mockBuilder, /* excludeLocalAddresses= */ false);

    verify(mockBuilder).addRoute(anyString(), anyInt());
    verifyNoMoreInteractions(mockBuilder);
  }

  @Test
  public void addIpv6Routes_excludeLocalAddressesAddsMultipleRoutes() {
    RouteManager.addIpv6Routes(mockBuilder, /* excludeLocalAddresses= */ true);

    verify(mockBuilder, atLeast(2)).addRoute(anyString(), anyInt());
    verifyNoMoreInteractions(mockBuilder);
  }
}
