#ifndef CHROME_BROWSER_UI_VIEWS_TOOLBAR_VIEW_NFS_H_
#define CHROME_BROWSER_UI_VIEWS_TOOLBAR_VIEW_NFS_H_

#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/views/search_bar/search_bar_view.h"
#include "ui/views/controls/button/image_button.h"
#include "chrome/browser/ui/views/toolbar/app_manager_button.h"
#include "chrome/browser/ui/views/toolbar/download_button.h"
#include "chrome/browser/ui/views/toolbar/wechat_button.h"
#include "chrome/browser/ui/views/toolbar/revocation_button.h"

class ScreenCaptureButton;
class DownloadButton;
class AppManagerButton;

class ToolbarViewNfs: public ToolbarView {
public:
  ToolbarViewNfs(Browser* browser);
  ~ToolbarViewNfs() override;

  SearchBarView* search_bar() const { return search_bar_; }

  DownloadButton* download_button() const { return download_button_; }
  WechatButton* wechat_button() const { return wechat_button_; }//yana 170310

  // Overridden from ToolbarView
  void Layout() override;
  void Init() override;
  void LoadImages() override;

  // Overridden from views::MenuButtonListener:
  // void OnMenuButtonClicked(views::View* source, const gfx::Point& point) override;

  gfx::Size GetPreferredSize() const override;

private:
  void OnShowSearchBarChanged();

  BooleanPrefMember show_search_bar_;
  BooleanPrefMember show_toolbar_;
  SearchBarView* search_bar_;
  DownloadButton* download_button_;
  AppManagerButton* app_manager_button_;
  WechatButton* wechat_button_;//yana 170309
  RevocationButton* revocation_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ToolbarViewNfs);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOOLBAR_VIEW_Nfs_H_
