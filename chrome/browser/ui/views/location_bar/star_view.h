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

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_STAR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_STAR_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/location_bar/bubble_icon_view.h"

class Browser;
class CommandUpdater;

// The star icon to show a bookmark bubble.
class StarView : public BubbleIconView {
 public:
  StarView(CommandUpdater* command_updater, Browser* browser);
  ~StarView() override;

  // Toggles the star on or off.
  void SetToggled(bool on);

 protected:
  // BubbleIconView:
  void OnExecuting(BubbleIconView::ExecuteSource execute_source) override;
  void ExecuteCommand(ExecuteSource source) override;
  views::BubbleDialogDelegateView* GetBubble() const override;
  views::BubbleDelegateView* GetCommonBubble() const override;
  gfx::VectorIconId GetVectorIcon() const override;
  // huk
  bool SetRasterIcon() override;

 private:
  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(StarView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_STAR_VIEW_H_
