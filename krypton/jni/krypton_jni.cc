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

// Please have all JNI methods listed below.  "extern C" is needed to ensure
// it's C method that Java can call.
#include <jni.h>
#include <jni_md.h>

#include <memory>
#include <string>

#include "base/logging.h"
#include "privacy/net/krypton/jni/http_fetcher.h"
#include "privacy/net/krypton/jni/jni_cache.h"
#include "privacy/net/krypton/jni/jni_timer_interface_impl.h"
#include "privacy/net/krypton/jni/jni_utils.h"
#include "privacy/net/krypton/jni/krypton_notification.h"
#include "privacy/net/krypton/jni/oauth.h"
#include "privacy/net/krypton/jni/vpn_service.h"
#include "privacy/net/krypton/krypton.h"
#include "privacy/net/krypton/proto/connection_status.proto.h"
#include "privacy/net/krypton/proto/debug_info.proto.h"
#include "privacy/net/krypton/proto/krypton_telemetry.proto.h"
#include "privacy/net/krypton/proto/network_info.proto.h"
#include "privacy/net/krypton/proto/network_type.proto.h"
#include "privacy/net/krypton/timer_manager.h"
#include "third_party/absl/status/status.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/time/time.h"

using privacy::krypton::IpSecTransformParams;
using privacy::krypton::Krypton;
using privacy::krypton::KryptonConfig;
using privacy::krypton::KryptonDebugInfo;
using privacy::krypton::KryptonTelemetry;
using privacy::krypton::NetworkInfo;
using privacy::krypton::NetworkType;
using privacy::krypton::TimerManager;
using privacy::krypton::jni::ConvertJavaByteArrayToString;
using privacy::krypton::jni::HttpFetcher;
using privacy::krypton::jni::JavaByteArray;
using privacy::krypton::jni::JniCache;
using privacy::krypton::jni::JniTimerInterfaceImpl;
using privacy::krypton::jni::KryptonNotification;
using privacy::krypton::jni::OAuth;
using privacy::krypton::jni::VpnService;

// Implementations of native methods from KryptonImpl.java.
// LINT.IfChange
extern "C" {

// Initialize the Krypton library.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_init(
    JNIEnv* env, jobject krypton_instance);

// Start the Krypton library.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_startNative(
    JNIEnv* env, jobject krypton_instance, jbyteArray config_bytes);

// Stop the Krypton library.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_stop(
    JNIEnv* env, jobject krypton_instance);

// Switch API for switching across different access networks.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_setNetworkNative(
    JNIEnv* env, jobject krypton_instance, jbyteArray request_byte_array);

// setNoNetworkAvailable API indicating there are no active networks.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_setNoNetworkAvailable(
    JNIEnv* env, jobject krypton_instance);

// Timer expiry
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_timerExpired(
    JNIEnv* env, jobject krypton_instance, int timer_id);

// Pause
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_pause(
    JNIEnv* env, jobject krypton_instance, int duration_msecs);

// CollectTelemetry
JNIEXPORT jbyteArray JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_collectTelemetryNative(
    JNIEnv* env, jobject krypton_instance);

// GetDebugInfo
JNIEXPORT jbyteArray JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_getDebugInfoNative(
    JNIEnv* env, jobject krypton_instance);
}
// LINT.ThenChange(//depot/google3/java/com/google/android/libraries/privacy/ppn/krypton/KryptonImpl.java)

namespace {

// Local scope krypton instance.  There should only be one krypton instance
// running at any given time.
struct KryptonCache {
 public:
  KryptonCache() {
    http_fetcher = absl::make_unique<HttpFetcher>();
    notification = absl::make_unique<KryptonNotification>();
    vpn_service = absl::make_unique<VpnService>();
    oauth = absl::make_unique<OAuth>();
    jni_timer_interface = absl::make_unique<JniTimerInterfaceImpl>();
    timer_manager = absl::make_unique<TimerManager>(jni_timer_interface.get());
  }

  ~KryptonCache() {
    // Order of deletion matters as there could be some pending API calls. Start
    // with Krypton.
    krypton.reset();
    notification.reset();
    vpn_service.reset();
    oauth.reset();
    http_fetcher.reset();
    // jni_timer_interface needs to be reset so that we don't get notifications
    // from Java.
    jni_timer_interface.reset();
    timer_manager.reset();
  }
  std::unique_ptr<JniTimerInterfaceImpl> jni_timer_interface;
  std::unique_ptr<TimerManager> timer_manager;
  std::unique_ptr<Krypton> krypton;
  std::unique_ptr<HttpFetcher> http_fetcher;
  std::unique_ptr<KryptonNotification> notification;
  std::unique_ptr<VpnService> vpn_service;
  std::unique_ptr<OAuth> oauth;
};

std::unique_ptr<KryptonCache> krypton_cache;

}  // namespace

// Krypton Initialization
// If Init is called when there is an active krypton instance, the older
// instance is terminated and a new gets to start.
JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_init(
    JNIEnv* env, jobject krypton_instance) {
  // Fetch the VM and store the krypton java object
  auto jni_ppn = privacy::krypton::jni::JniCache::Get();
  jni_ppn->Init(env, krypton_instance);

  // Initialize the Krypton library.
  LOG(INFO) << "Initializing the Krypton native library";
  if (krypton_cache != nullptr) {
    LOG(INFO) << "Resetting the cached Krypton instance.";
    krypton_cache->krypton->Stop();
    krypton_cache.reset();
  }

  // Create the new Krypton object and make it the singleton.
  krypton_cache = absl::make_unique<KryptonCache>();
  krypton_cache->krypton = absl::make_unique<privacy::krypton::Krypton>(
      krypton_cache->http_fetcher.get(), krypton_cache->notification.get(),
      krypton_cache->vpn_service.get(), krypton_cache->oauth.get(),
      krypton_cache->timer_manager.get());
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_startNative(
    JNIEnv* env, jobject krypton_instance, jbyteArray config_byte_array) {
  LOG(INFO) << "Starting Krypton native library";
  if (krypton_cache == nullptr || krypton_cache->krypton == nullptr) {
    JniCache::Get()->ThrowKryptonException("Krypton was not initialized.");
    return;
  }

  // Parse the config.
  KryptonConfig config;
  std::string config_bytes =
      ConvertJavaByteArrayToString(env, config_byte_array);
  if (!config.ParseFromString(config_bytes)) {
    JniCache::Get()->ThrowKryptonException("invalid KryptonConfig bytes");
    return;
  }

  krypton_cache->krypton->Start(config);
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_stop(
    JNIEnv* env, jobject /*thiz*/) {
  // Initialize the Krypton library.
  LOG(INFO) << "Stopping Krypton native library";
  if (krypton_cache != nullptr) {
    krypton_cache->krypton->Stop();
    krypton_cache.reset();
  }
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_setNoNetworkAvailable(
    JNIEnv* env, jobject krypton_instance) {
  LOG(INFO) << "SetNoNetworkAvailable is called";

  if (krypton_cache == nullptr || krypton_cache->krypton == nullptr) {
    JniCache::Get()->ThrowKryptonException("Krypton is not running");
    return;
  }

  auto status = krypton_cache->krypton->SetNoNetworkAvailable();
  if (!status.ok()) {
    JniCache::Get()->ThrowKryptonException(status.ToString());
    return;
  }
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_setNetworkNative(
    JNIEnv* env, jobject krypton_instance, jbyteArray request_byte_array) {
  NetworkInfo request;
  std::string request_bytes =
      ConvertJavaByteArrayToString(env, request_byte_array);
  if (!request.ParseFromString(request_bytes)) {
    JniCache::Get()->ThrowKryptonException("invalid NetworkInfo bytes");
    return;
  }

  if (krypton_cache == nullptr || krypton_cache->krypton == nullptr) {
    JniCache::Get()->ThrowKryptonException("Krypton is not running");
    return;
  }

  auto status = krypton_cache->krypton->SetNetwork(request);
  if (!status.ok()) {
    JniCache::Get()->ThrowKryptonException(status.ToString());
    return;
  }
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_timerExpired(
    JNIEnv* env, jobject krypton_instance, int timer_id) {
  if (krypton_cache == nullptr || krypton_cache->timer_manager == nullptr) {
    JniCache::Get()->ThrowKryptonException(
        "Krypton or TimerManager is not running");
    return;
  }
  krypton_cache->jni_timer_interface->TimerExpiry(timer_id);
}

JNIEXPORT void JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_pause(
    JNIEnv* env, jobject krypton_instance, int duration_msecs) {
  if (krypton_cache == nullptr || krypton_cache->timer_manager == nullptr) {
    JniCache::Get()->ThrowKryptonException(
        "Krypton or TimerManager is not running");
    return;
  }
  auto duration = absl::Milliseconds(duration_msecs);
  auto status = krypton_cache->krypton->Pause(duration);
  if (!status.ok()) {
    JniCache::Get()->ThrowKryptonException(status.ToString());
    return;
  }
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_collectTelemetryNative(
    JNIEnv* env, jobject krypton_instance) {
  LOG(INFO) << "collectTelemetry is called";

  if (krypton_cache == nullptr || krypton_cache->krypton == nullptr) {
    JniCache::Get()->ThrowKryptonException("Krypton is not running");
    return env->NewByteArray(0);
  }

  KryptonTelemetry telemetry;
  krypton_cache->krypton->CollectTelemetry(&telemetry);
  std::string bytes = telemetry.SerializeAsString();

  // We can't use JavaArray here, because we need to return the local reference.
  jbyteArray array = env->NewByteArray(bytes.size());
  env->SetByteArrayRegion(array, 0, bytes.size(),
                          reinterpret_cast<const jbyte*>(bytes.data()));
  return array;
}

JNIEXPORT jbyteArray JNICALL
Java_com_google_android_libraries_privacy_ppn_krypton_KryptonImpl_getDebugInfoNative(
    JNIEnv* env, jobject krypton_instance) {
  LOG(INFO) << "getDebugInfoBytes is called";

  if (krypton_cache == nullptr || krypton_cache->krypton == nullptr) {
    JniCache::Get()->ThrowKryptonException("Krypton is not running");
    return env->NewByteArray(0);
  }

  KryptonDebugInfo debug_info;
  krypton_cache->krypton->GetDebugInfo(&debug_info);
  std::string bytes = debug_info.SerializeAsString();

  // We can't use JavaArray here, because we need to return the local reference.
  jbyteArray array = env->NewByteArray(bytes.size());
  env->SetByteArrayRegion(array, 0, bytes.size(),
                          reinterpret_cast<const jbyte*>(bytes.data()));
  return array;
}
