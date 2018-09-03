/**
 * Copyright (c) 2016-2018 CPU and Fundamental Software Research Center, CAS
 *
 * This software is published by the license of CPU-OS Licence, you can use and
 * distribute this software under this License. See CPU-OS License for more detail.
 *
 * You should have received a copy of CPU-OS License. If not, please contact us
 * by email <support_os@cpu-os.ac.cn>
 *
**/

#include "chrome/browser/ui/views/location_bar/star_view.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/view_ids.h"
//#include "chrome/browser/ui/views/bookmarks/bookmark_bubble_view.h"
#include "chrome/browser/ui/views/bookmarks/nfsbrowser_bookmark_bubble_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "grit/ui_resources_nfs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/vector_icons_public.h"
#include "ui/base/resource/resource_bundle.h"

StarView::StarView(CommandUpdater* command_updater, Browser* browser)
    : BubbleIconView(command_updater, IDC_BOOKMARK_PAGE), browser_(browser) {
  set_id(VIEW_ID_STAR_BUTTON);
  SetToggled(false);
}

StarView::~StarView() {}

void StarView::SetToggled(bool on) {
  BubbleIconView::SetActiveInternal(on);
  SetTooltipText(l10n_util::GetStringUTF16(
      on ? IDS_TOOLTIP_STARRED : IDS_TOOLTIP_STAR));
}

void StarView::OnExecuting(
    BubbleIconView::ExecuteSource execute_source) {
  BookmarkEntryPoint entry_point = BOOKMARK_ENTRY_POINT_STAR_MOUSE;
  switch (execute_source) {
    case EXECUTE_SOURCE_MOUSE:
      entry_point = BOOKMARK_ENTRY_POINT_STAR_MOUSE;
      break;
    case EXECUTE_SOURCE_KEYBOARD:
      entry_point = BOOKMARK_ENTRY_POINT_STAR_KEY;
      break;
    case EXECUTE_SOURCE_GESTURE:
      entry_point = BOOKMARK_ENTRY_POINT_STAR_GESTURE;
      break;
  }
  UMA_HISTOGRAM_ENUMERATION("Bookmarks.EntryPoint",
                            entry_point,
                            BOOKMARK_ENTRY_POINT_LIMIT);
}

void StarView::ExecuteCommand(ExecuteSource source) {
  if (browser_) {
    OnExecuting(source);
    chrome::BookmarkCurrentPageIgnoringExtensionOverrides(browser_);
  } else {
    BubbleIconView::ExecuteCommand(source);
  }
}

views::BubbleDialogDelegateView* StarView::GetBubble() const {
  //return BookmarkBubbleView::bookmark_bubble();
  //We use BubbleDelegateView in nfs bookmark bubble, so return nullprt here.
  return nullptr;
}

views::BubbleDelegateView* StarView::GetCommonBubble() const {
  return BookmarkBubbleView::bookmark_bubble();
}

gfx::VectorIconId StarView::GetVectorIcon() const {
  return active() ? gfx::VectorIconId::LOCATION_BAR_STAR_ACTIVE
                  : gfx::VectorIconId::LOCATION_BAR_STAR;
}

bool StarView::SetRasterIcon() {
  if (!hovered()) {
    SetImage(ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      active() ? IDD_ADDFAV_ACTIVE_N : IDD_ADDFAV_N));
  } else {
    SetImage(ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      active() ? IDD_ADDFAV_ACTIVE_H : IDD_ADDFAV_H));
  }
  return true;
}
