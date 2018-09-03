#include "chrome/browser/ui/views/toolbar/toolbar_view_nfs.h"

#include "base/command_line.h"
#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/autofill/save_card_icon_view.h"
#include "chrome/browser/ui/views/extensions/extension_popup.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/page_action_image_view.h"
#include "chrome/browser/ui/views/location_bar/page_action_with_badge_view.h"
#include "chrome/browser/ui/views/location_bar/star_view.h"
#include "chrome/browser/ui/views/outdated_upgrade_bubble_view.h"
#include "chrome/browser/ui/views/search_bar/search_bar_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/back_button.h"
#include "chrome/browser/ui/views/toolbar/browser_actions_container.h"
#include "chrome/browser/ui/views/toolbar/home_button.h"
#include "chrome/browser/ui/views/toolbar/reload_button.h"
// #include "chrome/browser/ui/views/toolbar/revocation_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/translate/translate_icon_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "grit/components_strings.h"
#include "grit/theme_resources.h"
#include "grit/ui_resources_nfs.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/base/window_open_disposition.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/vector_icons_public.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/views/focus/view_storage.h"
#include "ui/views/view_targeter.h"
#include "ui/views/widget/tooltip_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#if defined(OS_WIN)
#include "chrome/browser/recovery/recovery_install_global_error_factory.h"
#include "chrome/browser/ui/views/conflicting_module_view_win.h"
#include "chrome/browser/ui/views/critical_notification_bubble_view.h"
#endif

#if !defined(OS_CHROMEOS)
#include "chrome/browser/signin/signin_global_error_factory.h"
#include "chrome/browser/sync/sync_global_error_factory.h"
#endif

#include "chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.h"

#if defined(USE_ASH)
#include "ash/shell.h"
#endif

#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/pre_install/preinstall_crx_info.h"

int CenteredChildY(int parent_height, int child_height) {
  int roundoff_amount = ui::MaterialDesignController::IsModeMaterial() ? 0 : 1;
  return (parent_height - child_height + roundoff_amount) / 2;
}

ToolbarViewNfs::ToolbarViewNfs(Browser* browser)
  : ToolbarView(browser),
    search_bar_(NULL),
    download_button_(NULL),
    app_manager_button_(NULL),
    wechat_button_(NULL),//yana 170310
    revocation_(NULL) {

  show_search_bar_.Init(
  prefs::kShowSearchBar,
  browser_->profile()->GetOriginalProfile()->GetPrefs(),
  base::Bind(&ToolbarViewNfs::Layout,
             base::Unretained(this)));

  show_toolbar_.Init(
  prefs::kShowToolbar,
  browser_->profile()->GetOriginalProfile()->GetPrefs(),
  base::Bind(&ToolbarViewNfs::Layout,
             base::Unretained(this)));
}

ToolbarViewNfs::~ToolbarViewNfs() {
}

void ToolbarViewNfs::Layout() {
  if (!is_display_mode_normal()) {
    ToolbarView::Layout();
    return;
  }

  // If we have not been initialized yet just do nothing.
  if (back_ == NULL)
    return;

  // We assume all child elements except the location bar are the same height.
  // Set child_y such that buttons appear vertically centered.
  int child_height =
      std::min(back_->GetPreferredSize().height(), height());
  int child_y = CenteredChildY(height(), child_height);

  // If the window is maximized, we extend the back button to the left so that
  // clicking on the left-most pixel will activate the back button.
  // TODO(abarth):  If the window becomes maximized but is not resized,
  //                then Layout() might not be called and the back button
  //                will be slightly the wrong size.  We should force a
  //                Layout() in this case.
  //                http://crbug.com/5540
  bool maximized = GetWidget()->IsMaximized();
  int back_width = back_->GetPreferredSize().width();
  const gfx::Insets insets(GetLayoutInsets(TOOLBAR));
  if (maximized) {
    back_->SetBounds(0, child_y, back_width + insets.left(), child_height);
    back_->SetLeadingMargin(insets.left());
  } else {
    back_->SetBounds(insets.left(), child_y, back_width, child_height);
    back_->SetLeadingMargin(0);
  }
  const int element_padding = GetLayoutConstant(TOOLBAR_ELEMENT_PADDING);
  int next_element_x = back_->bounds().right() + element_padding;

  forward_->SetBounds(next_element_x, child_y,
                      forward_->GetPreferredSize().width(), child_height);
  next_element_x = forward_->bounds().right() + element_padding;

  revocation_->SetBounds(next_element_x, child_y,
                     revocation_->GetPreferredSize().width(), child_height);
  next_element_x = revocation_->bounds().right();

  next_element_x += element_padding;

  int app_menu_width = app_menu_button_->GetPreferredSize().width();


  // huk search bar may not be visible
  bool search_bar_visible = browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowSearchBar);
  int search_bar_width = search_bar_visible ? search_bar_->GetPreferredSize().width() : 0;
  int download_width = browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar) ?
      download_button_->GetPreferredSize().width() : 0;
  download_button_->SetVisible(browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar));
  int app_manager_width = browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar) ?
       app_manager_button_->GetPreferredSize().width() : 0;
  app_manager_button_->SetVisible(browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar));
  // int app_manager_width = 0; //暂时隐藏应用中心
  // app_manager_button_->SetVisible(false); //暂时隐藏应用中心
/*************************yana 170310*****************************/

  ExtensionService* service =
      extensions::ExtensionSystem::Get(browser_->profile())->extension_service();

  bool wechat_visible = service->GetInstalledExtension(extensions::GetWechatID()) &&
                        service->IsExtensionEnabled(extensions::GetWechatID());
  wechat_visible &= (browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar));

  int wechat_width = wechat_visible ?
      wechat_button_->GetPreferredSize().width() : 0;
  wechat_button_->SetVisible(wechat_visible);
/*************************yana 170310*****************************/

  int kExtraSpace = 4;
  if (!browser_actions_->width() && !wechat_visible) {
    kExtraSpace = 10;
  }
  int available_width = std::max(
        0, width() - app_menu_width - search_bar_width - download_width  - app_manager_width - wechat_width - next_element_x - kExtraSpace);

  int browser_action_width =   browser_actions_->GetWidthForMaxWidth(
      available_width - location_bar_->GetMinimumSize().width());

  if (!browser_->profile()->GetPrefs()->GetBoolean(prefs::kShowToolbar))  {
    browser_action_width = 0;
  }

  available_width -= browser_action_width;

  // Don't allow the omnibox to shrink to the point of non-existence, so
  // subtract its minimum width from the available width to reserve it.
  int location_bar_width = available_width;

  int location_height = location_bar_->GetPreferredSize().height();
  int location_y = CenteredChildY(height(), location_height);

  location_bar_->SetBounds(next_element_x, location_y,
                           location_bar_width, location_height);

  next_element_x = location_bar_->bounds().right();

  search_bar_->SetBounds(next_element_x, location_y,
                         search_bar_width, location_height);

  // 从右边往左排
  int app_menu_left = x() + width() - app_menu_width - 3;
  next_element_x = app_menu_left;

  app_menu_button_->SetBounds(next_element_x, child_y, app_menu_width,
                              child_height);

  if (download_button_ && download_button_->visible())  {
    next_element_x -= (download_width - 3);
    download_button_->SetBounds(next_element_x, location_y , download_width,
                              location_height);
  }

   if (app_manager_button_ && app_manager_button_->visible())  {
    next_element_x -= (app_manager_width + 3);
    app_manager_button_->SetBounds(next_element_x, location_y , app_manager_width,
                              location_height);
  }
/*************************yana 170310*****************************/
  if (wechat_button_ && wechat_button_->visible())  {
    next_element_x -= (wechat_width + 3);
    wechat_button_->SetBounds(next_element_x, location_y , wechat_width,
                              location_height);
  }
/*************************yana 170310*****************************/


  if (browser_action_width > 0)  {
    next_element_x -= browser_action_width;
    browser_actions_->SetBounds(next_element_x, child_y + 5, browser_action_width, child_height);
  } else {
    browser_actions_->SetBounds(next_element_x, child_y + 5, 0, 0);
  }

  bool origin_visibility = browser_action_width > 0;

  int browser_action_rightmost = wechat_button_->visible() ? wechat_button_->x() : app_manager_button_->x();
  // int browser_action_rightmost = wechat_button_->visible() ? wechat_button_->x() : download_button_->x(); //暂时隐藏应用中心

  if (browser_actions_->x() + browser_actions_->width() > browser_action_rightmost)  {
    browser_actions_->SetVisible(false);
  } else {
    browser_actions_->SetVisible(origin_visibility);
  }
}

void ToolbarViewNfs::Init() {
  ToolbarView::Init();

  if (!is_display_mode_normal()) {
    return;
  }

  download_button_ = new DownloadButton(browser_, this);
  download_button_->SetTooltipText(l10n_util::GetStringUTF16(IDS_DOWNLOAD_TOOLTIP));
  download_button_->set_id(VIEW_ID_DOWNLOAD_BUTTON);
  download_button_->SetVisible(true);

  app_manager_button_ = new AppManagerButton(browser_, this);
  app_manager_button_->SetTooltipText(l10n_util::GetStringUTF16(IDS_APP_MANAGER_TOOLTIP));
  app_manager_button_->set_id(VIEW_ID_APP_MANAGER_BUTTON);
  app_manager_button_->SetVisible(true);

   // we wanna to keep the correct order
  int index = GetIndexOf(app_menu_button_);
  DCHECK(index != -1);
  AddChildViewAt(download_button_, index);

  AddChildView(app_manager_button_);

/*************************yana 170310*****************************/
  wechat_button_ = new WechatButton(browser_, this);
  wechat_button_->SetTooltipText(l10n_util::GetStringUTF16(IDS_WECHAT_TOOLTIP));
  wechat_button_->set_id(VIEW_ID_WECHAT_BUTTON);
  wechat_button_->SetVisible(true);

  AddChildViewAt(wechat_button_, index);
/*************************yana 170310*****************************/

  search_bar_ = new SearchBarView(
        browser_,
        browser_->profile(),
        browser_->command_controller()->command_updater(),
        this);
  AddChildView(search_bar_);
  search_bar_->Init();

  revocation_ = new RevocationButton(
      browser_, this,
      new RecentTabsSubMenuModel(this,browser_,NULL));
  revocation_->set_hide_ink_drop_when_showing_context_menu(false);
  revocation_->set_triggerable_event_flags(
      ui::EF_LEFT_MOUSE_BUTTON | ui::EF_MIDDLE_MOUSE_BUTTON);
  revocation_->set_tag(IDC_REVOCATION);
  revocation_->SetTooltipText(l10n_util::GetStringUTF16(IDS_TOOLTIP_WEB_RECYCLE_BIN));
  //revocation_->SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_REVOCATION));
  //revocation_->set_id(VIEW_ID_BACK_BUTTON);
  AddChildView(revocation_);
  revocation_->Init();

  LoadImages();
}


void ToolbarViewNfs::LoadImages() {
  ToolbarView::LoadImages();
  if (!is_display_mode_normal())  {
    return;
  }

  const ui::ThemeProvider* tp = GetThemeProvider();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  bool dark = tp->GetDisplayProperty(ThemeProperties::THEME_ICONS_DARK) ? true:false;
  int id_menu_n, id_menu_h, id_menu_p;
  if(browser_->profile()->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    id_menu_n = dark ? IDD_MENU_INVISIBLE_N_DARK : IDD_MENU_INVISIBLE_N;
    id_menu_h = dark ? IDD_MENU_INVISIBLE_H_DARK : IDD_MENU_INVISIBLE_H;
    id_menu_p = dark ? IDD_MENU_INVISIBLE_P_DARK : IDD_MENU_INVISIBLE_P;
  } else {
    id_menu_n = dark ? IDD_MENU_N_DARK : IDD_MENU_N;
    id_menu_h = dark ? IDD_MENU_H_DARK : IDD_MENU_H;
    id_menu_p = dark ? IDD_MENU_P_DARK : IDD_MENU_P;
  }
  int id_back_n = dark ? IDD_BACK_N_DARK : IDD_BACK_N;
  int id_back_d = dark ? IDD_BACK_D_DARK : IDD_BACK_D;
  int id_back_h = dark ? IDD_BACK_H_DARK : IDD_BACK_H;
  int id_back_p = dark ? IDD_BACK_P_DARK : IDD_BACK_P;
  int id_forward_n = dark ? IDD_FORWARD_N_DARK : IDD_FORWARD_N;
  int id_forward_d = dark ? IDD_FORWARD_D_DARK : IDD_FORWARD_D;
  int id_forward_h = dark ? IDD_FORWARD_H_DARK : IDD_FORWARD_H;
  int id_forward_p = dark ? IDD_FORWARD_P_DARK : IDD_FORWARD_P;
  int id_revocation_n = dark ? IDD_REVOCATION_N_DARK : IDD_REVOCATION_N;
  int id_revocation_d = dark ? IDD_REVOCATION_D_DARK : IDD_REVOCATION_D;
  int id_revocation_h = dark ? IDD_REVOCATION_H_DARK : IDD_REVOCATION_H;
  int id_revocation_p = dark ? IDD_REVOCATION_P_DARK : IDD_REVOCATION_P;

/*************************yana 170310*****************************/
  wechat_button_->SetImage(views::Button::STATE_NORMAL,
                             rb.GetImageNamed(IDD_WECHAT_N).ToImageSkia());

  wechat_button_->SetImage(views::Button::STATE_HOVERED,
                             rb.GetImageNamed(IDD_WECHAT_H).ToImageSkia());
/*************************yana 170310*****************************/

  app_menu_button_->SetImage(views::Button::STATE_NORMAL,
                            *(rb.GetImageNamed(id_menu_n).ToImageSkia()));
  app_menu_button_->SetImage(views::Button::STATE_HOVERED,
                            *(rb.GetImageNamed(id_menu_h).ToImageSkia()));
  app_menu_button_->SetImage(views::Button::STATE_PRESSED,
                            *(rb.GetImageNamed(id_menu_p).ToImageSkia()));

  back_->SetImage(views::Button::STATE_NORMAL,
                  *(rb.GetImageNamed(id_back_n).ToImageSkia()));
  back_->SetImage(views::Button::STATE_DISABLED,
                  *(rb.GetImageNamed(id_back_d).ToImageSkia()));
  back_->SetImage(views::Button::STATE_HOVERED,
                  *(rb.GetImageNamed(id_back_h).ToImageSkia()));
  back_->SetImage(views::Button::STATE_PRESSED,
                  *(rb.GetImageNamed(id_back_p).ToImageSkia()));

  forward_->SetImage(views::Button::STATE_NORMAL,
                  *(rb.GetImageNamed(id_forward_n).ToImageSkia()));
  forward_->SetImage(views::Button::STATE_DISABLED,
                  *(rb.GetImageNamed(id_forward_d).ToImageSkia()));
  forward_->SetImage(views::Button::STATE_HOVERED,
                  *(rb.GetImageNamed(id_forward_h).ToImageSkia()));
  forward_->SetImage(views::Button::STATE_PRESSED,
                  *(rb.GetImageNamed(id_forward_p).ToImageSkia()));

  revocation_->SetImage(views::Button::STATE_NORMAL,
                  *(rb.GetImageNamed(id_revocation_n).ToImageSkia()));
  revocation_->SetImage(views::Button::STATE_DISABLED,
                  *(rb.GetImageNamed(id_revocation_d).ToImageSkia()));
  revocation_->SetImage(views::Button::STATE_HOVERED,
                  *(rb.GetImageNamed(id_revocation_h).ToImageSkia()));
  revocation_->SetImage(views::Button::STATE_PRESSED,
                  *(rb.GetImageNamed(id_revocation_p).ToImageSkia()));

  const SkColor normal_color =
      tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  back_->set_ink_drop_base_color(normal_color);
  forward_->set_ink_drop_base_color(normal_color);
  revocation_->set_ink_drop_base_color(normal_color);
  app_menu_button_->set_ink_drop_base_color(normal_color);
}


gfx::Size ToolbarViewNfs::GetPreferredSize() const {
  gfx::Size size(location_bar_->GetPreferredSize());
  if (is_display_mode_normal())  {
    const int element_padding = GetLayoutConstant(TOOLBAR_ELEMENT_PADDING);
    const int content_width =
            GetLayoutInsets(TOOLBAR).width() +
            back_->GetPreferredSize().width() + element_padding +
            forward_->GetPreferredSize().width() + element_padding +
            revocation_->GetPreferredSize().width() + element_padding +
            GetLayoutConstant(TOOLBAR_STANDARD_SPACING) +
            GetLayoutConstant(TOOLBAR_LOCATION_BAR_RIGHT_PADDING) +
            app_menu_button_->GetPreferredSize().width() +
            element_padding + search_bar_->GetPreferredSize().width() +
            element_padding + (download_button_->visible() ? download_button_->GetPreferredSize().width()  : 0) +
            element_padding + (app_manager_button_->visible() ? app_manager_button_->GetPreferredSize().width()  : 0) + //暂时隐藏应用中心
            element_padding +(wechat_button_->visible() ? wechat_button_->GetPreferredSize().width()  : 0) +
            element_padding;
    size.Enlarge(content_width, 0);
  }

  return SizeForContentSize(size);
}
