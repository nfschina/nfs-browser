// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef External_h
#define External_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "platform/heap/Handle.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/DOMWindowProperty.h"

namespace blink {
class LocalFrame;

class CORE_EXPORT External : public GarbageCollected<External>, 
                                                               public ScriptWrappable,
                                                               public DOMWindowProperty {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(External);

 public:
  static External* create(LocalFrame* frame) { return new External(frame); }

  void AddSearchProvider();
  void IsSearchProviderInstalled();
  void addFavorite(const String& url, const String& title);
  void addfavorite(const String& url, const String& title);
  void AddFavorite(const String& url, const String& title);
  void Addfavorite(const String& url, const String& title);

  DECLARE_VIRTUAL_TRACE();

 private:
  explicit External(LocalFrame*);
  // Member<LocalFrame> m_frame;
};

}  // namespace blink

#endif  // External_h
