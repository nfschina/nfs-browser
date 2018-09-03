// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_QR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_QR_VIEW_H_

#include "ui/views/view.h"
#include "url/gurl.h"
#include "ui/gfx/canvas.h"
#include "third_party/QR/src/qrenc.h"

class Browser;

class QrView : public views::View {
 public:
  QrView(GURL& url);
  ~QrView() override; 

  void Init();
  QRcode* GetQrcode();

 protected:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size GetPreferredSize() const override;
  void ConfirmZoomSize();

 private:
   GURL url_;
   QRcode *qrcode_;
   float zoom_size_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_QR_VIEW _VIEW_H_
