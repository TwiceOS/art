/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>

#include "android-base/logging.h"
#include "jni.h"
#include "scoped_local_ref.h"
#include "scoped_primitive_array.h"

#include "jvmti.h"

// Test infrastructure
#include "jvmti_helper.h"
#include "test_env.h"

namespace art {
namespace Test1921NativeSuspendMonitorRecursive {

std::atomic<bool> should_pause(false);
std::atomic<bool> paused(false);
std::atomic<bool> done(false);
std::atomic<bool> started(false);

extern "C" JNIEXPORT void JNICALL Java_art_Test1921_nativeSpin(JNIEnv* env, jclass) {
  jrawMonitorID mon;
  if (JvmtiErrorToException(
      env, jvmti_env, jvmti_env->CreateRawMonitor("Test1921 monitor", &mon))) {
    return;
  }
  if (JvmtiErrorToException(env, jvmti_env, jvmti_env->RawMonitorEnter(mon))) {
    return;
  }
  while (!done.load()) {
    while (should_pause.load()) {
      paused.store(true);
    }
    paused.store(false);
    if (JvmtiErrorToException(env, jvmti_env, jvmti_env->RawMonitorEnter(mon))) {
      return;
    }
    started.store(true);
    if (JvmtiErrorToException(env, jvmti_env, jvmti_env->RawMonitorExit(mon))) {
      return;
    }
  }
  if (JvmtiErrorToException(env, jvmti_env, jvmti_env->RawMonitorExit(mon))) {
    return;
  }
}

extern "C" JNIEXPORT jboolean JNICALL Java_art_Test1921_isNativeThreadSpinning(JNIEnv*, jclass) {
  return started.load();
}

extern "C" JNIEXPORT void JNICALL Java_art_Test1921_pause(JNIEnv*, jclass) {
  should_pause.store(true);
  paused.store(false);
  while (!paused.load()) { }
}

extern "C" JNIEXPORT void JNICALL Java_art_Test1921_resume(JNIEnv*, jclass) {
  should_pause.store(false);
  while (paused.load()) { }
}

extern "C" JNIEXPORT void JNICALL Java_art_Test1921_reset(JNIEnv*, jclass) {
  started.store(false);
}

extern "C" JNIEXPORT void JNICALL Java_art_Test1921_nativeFinish(JNIEnv*, jclass) {
  done.store(true);
}

}  // namespace Test1921NativeSuspendMonitorRecursive
}  // namespace art
