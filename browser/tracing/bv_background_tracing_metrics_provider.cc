// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/tracing/bv_background_tracing_metrics_provider.h"

#include "bison/browser/metrics/bv_metrics_service_client.h"
#include "bison/browser/tracing/background_tracing_field_trial.h"
#include "base/strings/string_piece.h"
#include "components/metrics/field_trials_provider.h"
#include "components/metrics/metrics_service.h"

namespace tracing {

BvBackgroundTracingMetricsProvider::BvBackgroundTracingMetricsProvider() =
    default;
BvBackgroundTracingMetricsProvider::~BvBackgroundTracingMetricsProvider() =
    default;

void BvBackgroundTracingMetricsProvider::Init() {
  bison::MaybeSetupWebViewOnlyTracing();

  metrics::MetricsService* metrics =
      bison::BvMetricsServiceClient::GetInstance()
          ->GetMetricsService();
  DCHECK(metrics);

  system_profile_providers_.emplace_back(
      std::make_unique<variations::FieldTrialsProvider>(
          metrics->GetSyntheticTrialRegistry(), base::StringPiece()));
}

void BvBackgroundTracingMetricsProvider::ProvideEmbedderMetrics(
    metrics::ChromeUserMetricsExtension* uma_proto,
    base::HistogramSnapshotManager* snapshot_manager) {
  // Remove the package name according to the privacy requirements.
  // See go/public-webview-trace-collection.
  auto* system_profile = uma_proto->mutable_system_profile();
  system_profile->clear_app_package_name();
}

}  // namespace tracing
