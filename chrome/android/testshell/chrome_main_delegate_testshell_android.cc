// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/android/testshell/chrome_main_delegate_testshell_android.h"

#include "base/android/jni_android.h"
#include "chrome/browser/search_engines/template_url_prepopulate_data.h"

static const char kDefaultCountryCode[] = "US";

ChromeMainDelegateTestShellAndroid::ChromeMainDelegateTestShellAndroid() {
}

ChromeMainDelegateTestShellAndroid::~ChromeMainDelegateTestShellAndroid() {
}

bool ChromeMainDelegateTestShellAndroid::BasicStartupComplete(int* exit_code) {
  TemplateURLPrepopulateData::InitCountryCode(kDefaultCountryCode);
  return ChromeMainDelegateAndroid::BasicStartupComplete(exit_code);
}

bool ChromeMainDelegateTestShellAndroid::RegisterApplicationNativeMethods(
    JNIEnv* env) {
  return ChromeMainDelegateAndroid::RegisterApplicationNativeMethods(env);
}
