// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/External.h"
#include "core/frame/FrameHost.h"
#include "core/page/ChromeClient.h"

namespace blink {

  External::External(LocalFrame* frame)
      : DOMWindowProperty(frame) {}


  DEFINE_TRACE(External) {
    DOMWindowProperty::trace(visitor);
  }

  void External::AddSearchProvider() {
  }

  void External::IsSearchProviderInstalled() {
  }

  void External::addFavorite(const String& url, const String& title) {
    if (!frame())
      return;

    FrameHost* host = frame()->host();
    if (!host)
      return;

    host->chromeClient().openJavaScriptAddFavorite(frame(), url, title);
  }

  void External::addfavorite(const String& url, const String& title) {
    addFavorite(url, title);
  }

  void External::AddFavorite(const String& url, const String& title) {
    addFavorite(url, title);
  }

  void External::Addfavorite(const String& url, const String& title) {
    addFavorite(url, title);
  }

}  // namespace blink
