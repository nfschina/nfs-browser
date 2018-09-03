#include "chrome/browser/ui/views/location_bar/qr_bubble_view.h"
#include "grit/ui_resources.h"
#include "grit/generated_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "chrome/grit/generated_resources.h"
#include "ui/views/layout/grid_layout.h"

using views::ImageView;
using views::Label;
using views::GridLayout;
using views::ColumnSet;

QrBubbleView::QrBubbleView(
    QrButton* anchor_view, 
    Profile* profile,
    const GURL& url)
    :BubbleDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
    profile_(profile),
    url_(url) ,
    qr_view_(nullptr),
    anchor_view_(anchor_view),
    label_(nullptr) {
      DCHECK(profile);
      if(profile_) {
        set_margins(gfx::Insets(20, 20, 15, 20));
        set_anchor_view_insets(gfx::Insets(0, 0, 0, 0));
        Init();
      }
    }

QrBubbleView::~QrBubbleView() {
  anchor_view_->SetMouseReleasedAction(false);
  anchor_view_->SetNextAction(false);
}

void QrBubbleView::Init() {
  qr_view_ = new QrView(url_);
  GridLayout* layout = new GridLayout(this);
  SetLayoutManager(layout);
  ColumnSet* cs = layout->AddColumnSet(0);
  cs->AddColumn(GridLayout::LEADING, GridLayout::LEADING, 0, GridLayout::USE_PREF,
                    0, 0);
  layout->StartRow(0, 0);
  layout->AddView(qr_view_);
  layout->AddPaddingRow(0, 10);

  label_ = new views::Label();
  label_->SetText(l10n_util::GetStringUTF16(IDS_QR_BUBBLE_TITLE_CDOS));
  layout->StartRow(0, 0);
  layout->AddView(label_, 1, 1, GridLayout::CENTER, GridLayout::CENTER);
}

//static
 views::Widget* QrBubbleView::ShowBubble(QrButton* anchor_view,
                                    const gfx::Rect& anchor_rect,
                                    gfx::NativeView parent_window,
                                    Profile* profile,
                                    GURL& url) {
  QrBubbleView* qr_bubble_ = new QrBubbleView(anchor_view, profile, url);
  qr_bubble_->SetBoundsRect(gfx::Rect(0, 0, 100, 100));
  if (!anchor_view) {
    qr_bubble_->SetAnchorRect(anchor_rect);
    qr_bubble_->set_parent_window(parent_window);
  }
  views::Widget* qr_bubble_widget = views::BubbleDelegateView::CreateBubble(qr_bubble_);
  qr_bubble_widget->Show();

  return qr_bubble_->GetWidget();
}

