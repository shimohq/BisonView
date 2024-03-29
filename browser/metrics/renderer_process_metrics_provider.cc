// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/metrics/renderer_process_metrics_provider.h"

#include "bison/common/bv_switches.h"
#include "base/command_line.h"
#include "base/metrics/histogram_functions.h"

namespace {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum SingleOrMultiProcess {
  kSingleProcess = 0,
  kMultiProcess = 1,
  kMaxValue = kMultiProcess,
};

void RecordRendererProcessMetricsImpl() {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  DCHECK(command_line);
  bool multiProcess =
      command_line->HasSwitch(switches::kWebViewSandboxedRenderer);
  base::UmaHistogramEnumeration(
      "Android.WebView.SingleOrMultiProcess",
      static_cast<SingleOrMultiProcess>(multiProcess));
}

}  // namespace

namespace bison {

void RendererProcessMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  RecordRendererProcessMetricsImpl();
}

}  // namespace android_webview
