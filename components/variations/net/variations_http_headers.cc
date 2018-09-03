// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/net/variations_http_headers.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "components/google/core/browser/google_util.h"
#include "components/variations/variations_http_header_provider.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace variations {

namespace {

}  // namespace

void AppendVariationHeaders(const GURL& url,
                            bool incognito,
                            bool uma_enabled,
                            net::HttpRequestHeaders* headers) {
}

std::set<std::string> GetVariationHeaderNames() {
  std::set<std::string> headers;
  return headers;
}

namespace internal {

// static
bool ShouldAppendVariationHeaders(const GURL& url) {
  return false;
}

}  // namespace internal

}  // namespace variations
