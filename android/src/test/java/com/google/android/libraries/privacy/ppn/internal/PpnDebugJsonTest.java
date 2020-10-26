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

package com.google.android.libraries.privacy.ppn.internal;

import static com.google.common.truth.Truth.assertThat;

import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class PpnDebugJsonTest {
  @Test
  public void builder_populatesAllFields() throws JSONException {
    JSONObject service = new JSONObject();
    JSONObject krypton = new JSONObject();
    JSONObject xenon = new JSONObject();

    JSONObject debugInfo =
        new PpnDebugJson.Builder()
            .setServiceDebugJson(service)
            .setKryptonDebugJson(krypton)
            .setXenonDebugJson(xenon)
            .build();

    assertThat(service.getJSONObject(PpnDebugJson.SERVICE)).isSameInstanceAs(service);
    assertThat(service.getJSONObject(PpnDebugJson.KRYPTON)).isSameInstanceAs(krypton);
    assertThat(service.getJSONObject(PpnDebugJson.XENON)).isSameInstanceAs(xenon);
  }
}
