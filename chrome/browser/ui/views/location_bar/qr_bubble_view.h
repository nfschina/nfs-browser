// Copyright (c) 2016 The shanghaihufang Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_QR_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_QR_BUBBLE_VIEW_H_

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/location_bar/qr_view.h"
#include "chrome/browser/ui/views/location_bar/qr_button.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"

class QrBubbleView : public views::BubbleDelegateView {
  public:
    QrBubbleView(
      QrButton* anchor_view, 
                     Profile* profile,
                     const GURL& url);
    ~QrBubbleView() override;

   // If |anchor_view| is null, |anchor_rect| is used to anchor the bubble and
  // |parent_window| is used to ensure the bubble closes if the parent closes.
    static views::Widget* ShowBubble(QrButton* anchor_view,
                       const gfx::Rect& anchor_rect,
                       gfx::NativeView parent_window,
                       Profile* profile,
                       GURL& url);

    void Init() override;
    //void OnPaint(gfx::Canvas* canvas) override;
    //void PaintChildren(const ui::PaintContext& context)  override;

  private:
      Profile* profile_;
      GURL url_;
      views::View* qr_view_;
      QrButton* anchor_view_;
      views::Label* label_;
};

#endif