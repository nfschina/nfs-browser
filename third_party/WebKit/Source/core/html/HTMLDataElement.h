// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLDataElement_h
#define HTMLDataElement_h

#include "core/html/HTMLElement.h"

namespace blink {

class HTMLDataElement final : public HTMLElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  DECLARE_NODE_FACTORY(HTMLDataElement);

 private:
  explicit HTMLDataElement(Document&);
};

}  // namespace blink

#endif  // HTMLDataElement_h
