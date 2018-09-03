// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/qr_view.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point.h"

float qr_bubble_width = 125;

QrView::QrView(GURL& url)
  :url_(url),
  qrcode_(nullptr),
  zoom_size_(5) {
  Init();
}

QrView::~QrView() {
}

void QrView::Init() {
    qrcode_ = GetQrcode();
    ConfirmZoomSize();
}

QRcode* QrView::GetQrcode() {
  QRcode *qrcode = NULL;
  if(url_.is_empty()) {
    printf("url is empty\n");
    return NULL;
  }

  char* url = const_cast<char*>(url_.spec().c_str());
  int length = url_.spec().size();
  qrcode = encode(url, length);

  return qrcode;
}

gfx::Size QrView::GetPreferredSize() const {
  int realwidth = qrcode_->width * zoom_size_;
  return gfx::Size(realwidth, realwidth);
}

void QrView::ConfirmZoomSize() {
  if(!qrcode_) {
    return;
  }

  zoom_size_ = qr_bubble_width / qrcode_->width;
}

void QrView::OnPaint(gfx::Canvas* canvas) {
  SkPaint paint_white;
  SkPaint paint_black;
  paint_white.setColor(SK_ColorWHITE);
  paint_black.setColor(SK_ColorBLACK);
 
  if(qrcode_ == NULL) {
    printf("url is emptry");
    return;
  }

  int x, y, xx, yy;
  unsigned  char* p;

  /* data */
  p = qrcode_->data;
  for(y= 0; y < qrcode_->width; y++) {
    for(x=0; x<qrcode_->width; x++) {
      for(yy = 0; yy < zoom_size_; yy ++) {
        for(xx = 0; xx < zoom_size_; xx++) {
          if(*p&1) {
            canvas->DrawPoint(gfx::Point(x*zoom_size_ + xx, y*zoom_size_ + yy), paint_black);
           } else {
            canvas->DrawPoint(gfx::Point(x*zoom_size_ + xx, y*zoom_size_ + yy), paint_white);
          }
        }
      }
      p++;
    }
  }
}
