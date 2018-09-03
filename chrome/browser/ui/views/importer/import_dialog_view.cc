#include "chrome/browser/ui/views/importer/import_dialog_view.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_lock_dialog.h"
#include "chrome/browser/importer/importer_uma.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/user_metrics.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_utils.h"
#include "ui/resources/grit/ui_resources_nfs.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/radio_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

#include "chrome/browser/ui/startup/import_favorites_infobar_delegate.h" //wdq
//#include "chrome/browser/ui/startup/check_update_infobar_delegate.h" //wdq

#if defined(OS_LINUX)
#include <unistd.h>
#endif

using namespace views;
using base::string16;
using base::UserMetricsAction;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::BrowserThread;
using importer::ImportItem;

static ImporterList* g_importer_list = NULL;

namespace {

#if !defined(OS_WIN)
#pragma clang diagnostic push
#endif

#if !defined(OS_WIN)
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

  std::map<int ,int> images_list = {
  #if defined(OS_WIN)
    {importer::TYPE_IE, IDR_CDOS_IMPORTER_IE},
    {importer::TYPE_EDGE, IDR_CDOS_IMPORTER_EDGE},
  #endif
  #if defined(OS_MACOSX)
    {importer::TYPE_SAFARI, IDR_CDOS_IMPORTER_SAFARI}
  #endif
    {importer::TYPE_FIREFOX, IDR_CDOS_IMPORTER_FIREFOX},
    {importer::TYPE_CHROME, IDR_CDOS_IMPORTER_CHROME},
    {importer::TYPE_BOOKMARKS_FILE, IDR_CDOS_IMPORTER_HTML}
  };

#if !defined(OS_WIN)
#pragma clang diagnostic pop
#endif

  gfx::Image GetImageNamed(int id) {
    return ui::ResourceBundle::GetSharedInstance().GetImageNamed(id);
  }
}

void ShowImportSelectDialog(gfx::NativeWindow parent,
                            base::string16 title,
                            int width) {
  ImportDialogView::Show(parent, title, width);
  content::RecordAction(UserMetricsAction("ImportLockDialogView_Shown"));
}

class NfsLabelButton : public views::LabelButton {
public:
  NfsLabelButton(views::ButtonListener* listener,
                  const base::string16& text,
                  bool is_first_item)
    : views::LabelButton(listener, text)
    , selected_(is_first_item) {
  }

  ~NfsLabelButton() override {}

  void set_selected() { selected_ = true; }
  bool is_selected() { return selected_; }

private:
  // View:
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;

  bool selected_;

  DISALLOW_COPY_AND_ASSIGN(NfsLabelButton);
};

// static
void ImportDialogView::Show(gfx::NativeWindow parent,
                            base::string16 title,
                            int width) {
  views::NfsDialogDelegate::CreateNfsDialogWidget(
      new ImportDialogView(title, width), NULL, parent)->Show();
}

//static
void ImportDialogView::ShouldShowImportDialog(Browser* browser, Profile* profile) {
  if (!g_importer_list)  {
    g_importer_list = new ImporterList();    
  }
  const base::Closure closure;
  g_importer_list->DetectSourceProfiles(
      g_browser_process->GetApplicationLocale(),
      true, 
      base::Bind(&ImportDialogView::ShowImportFavoritesInfobar, browser, profile));
}

//static 
void ImportDialogView::ShowImportFavoritesInfobar(Browser* browser, Profile* profile) {
  if(g_importer_list->count() > 1) {
    chrome::ImportFavoritesInfoBarDelegate::Create(InfoBarService::FromWebContents(
        browser->tab_strip_model()->GetActiveWebContents()), profile);
  }
  delete g_importer_list;
  g_importer_list = NULL;
}

//static 
/*void ImportDialogView::ShowImportFavoritesInfobar(Browser* browser, Profile* profile) {
  chrome::CheckUpdateInfoBarDelegate::Create(InfoBarService::FromWebContents(
      browser->tab_strip_model()->GetActiveWebContents()), profile);
}*/

ImportDialogView::ImportDialogView(base::string16 title)
    : ImportDialogView(title, 350) {
}

ImportDialogView::ImportDialogView(base::string16 title, int width)
    : selected_count_(0)
    , profile_(ProfileManager::GetPrimaryUserProfile())
    , clear_(NULL)
    , information_(NULL)
    , importer_host_(NULL)
    , title_(title)
    , width_(width)
    , import_did_succeed_(false)
    , start_(false)
    , end_(false)
    , weak_ptr_factory_(this) {
  selected_.clear();

  importer_list_.reset(new ImporterList());
  importer_list_->DetectSourceProfiles(
      g_browser_process->GetApplicationLocale(),
      true,
      base::Bind(&ImportDialogView::GetSupportedBrowser, base::Unretained(this)));

  clear_ = new Checkbox(l10n_util::GetStringUTF16(IDS_CDOS_CLEAR_ALL_BOOKMARKS));

  AddChildView(clear_);

  //profile_->GetPrefs()->SetBoolean(prefs::kShouldShowImportDialog, false);
}

ImportDialogView::~ImportDialogView() {
  if (importer_host_)
    importer_host_->set_observer(NULL);
}

void ImportDialogView::RemoveBookmarks() {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);

  DCHECK(model);
  if (!model)
    return ;
  // model->RemoveAllUserBookmarks();
  BookmarkNode* node = const_cast<BookmarkNode*>(model->bookmark_bar_node());
  DCHECK(node);
  if (!node) {
    return ;
  }

  while(!node->empty()) {
    model->Remove(node->GetChild(0));
  }
  return ;
}

gfx::Size ImportDialogView::GetPreferredSize() const {
  return gfx::Size(width_, 50*browser_info_list_.size() + 90);
}

void ImportDialogView::Layout() {
  gfx::Rect bounds(GetLocalBounds());
  bounds.Inset(views::kButtonHEdgeMargin, views::kPanelVertMargin);
  size_t i = 0;
  for (BrowserInfoMap::iterator it=browser_info_list_.begin(); it!=browser_info_list_.end(); ++it) {
    it->second->browser_image->SetBounds(25, 10+50*i, width() - 25 * 2, 45);
    if (browser_info_list_.begin() == it) {
      it->second->browser_image->set_selected();
      it->second->browser_image->set_background(
          Background::CreateSolidBackground(222, 236, 255));
    }
    i++;
  }

  clear_->SetBounds(25, 10+50*i, width() - 25 * 2, 15);
}

base::string16 ImportDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16((button == ui::DIALOG_BUTTON_OK) ?
      IDS_IMPORTER_LOCK_OK : IDS_CDOS_IMPORT_LOCK_CANCEL);
}

base::string16 ImportDialogView::GetWindowTitle() const {
  if (title_.empty())
    return l10n_util::GetStringUTF16(IDS_CDOS_IMPORT_DIALOG_TITLE);
  return title_;
}

bool ImportDialogView::Accept() {
  if(profile_)
    profile_->GetPrefs()->SetBoolean(prefs::kShouldShowImportDialog, false);
  selected_count_ = 0;

  if (clear_->checked()) {
    RemoveBookmarks();
  }

  for (BrowserInfoMap::iterator it=browser_info_list_.begin(); it!=browser_info_list_.end(); ++it) {
    if (it->second->browser_image->is_selected()) {
      selected_count_++;
      selected_.push_back(it->second->index);
    }
  }

  StartImport();
  return false;
}

bool ImportDialogView::Cancel() {
  return true;
}

void ImportDialogView::GetSupportedBrowser() {
  for (size_t i = 0; i < importer_list_->count(); ++i) {
    const importer::SourceProfile& source_profile =
        importer_list_->GetSourceProfileAt(i);

    std::unique_ptr<BrowserInfo> browser_info;
    browser_info.reset(new BrowserInfo());
    browser_info->browser_image = new NfsLabelButton(
        this, source_profile.importer_name, false);
    browser_info->browser_image->SetImage(
        views::Button::STATE_NORMAL,
        GetImageNamed(images_list[source_profile.importer_type]).AsImageSkia());
    browser_info->browser_image->SetBorder(
        Border::CreateSolidBorder(1, SkColorSetRGB(242, 242, 242)));

    browser_info->browser_name = source_profile.importer_name;
    browser_info->browser_type = source_profile.importer_type;
    browser_info->index = i;

    AddChildView(browser_info->browser_image);

    browser_info_list_.insert(
        std::pair<base::string16, std::unique_ptr<BrowserInfo>>(
            browser_info->browser_name, std::move(browser_info)));
  }

  GetWidget()->SetSize(GetPreferredSize());

  Layout();
}

void ImportDialogView::Reset() {
  import_did_succeed_ = start_ = end_ = false;
  selected_count_ = 0;
  selected_.clear();

  browser_info_list_.clear();

  if (information_) {
    delete information_;
    information_ = NULL;
  }
  if (clear_) {
    delete clear_;
    clear_ = NULL;
  }
  clear_ = new Checkbox(l10n_util::GetStringUTF16(IDS_CDOS_CLEAR_ALL_BOOKMARKS));
  AddChildView(clear_);

  if (importer_host_) {
    importer_host_->set_observer(NULL);
    importer_host_ = NULL;
  }

  importer_list_.reset(new ImporterList());
  importer_list_->DetectSourceProfiles(
      g_browser_process->GetApplicationLocale(),
      true,
      base::Bind(&ImportDialogView::GetSupportedBrowser, base::Unretained(this)));

  const DialogClientView* dcv = static_cast<DialogClientView*>(dialog_client_view_);

  dcv->ok_button()->SetEnabled(true);
  dcv->cancel_button()->SetEnabled(true);
}

void ImportDialogView::StartImport() {
  if (selected_.empty())
    return ;

  size_t index = selected_[0];
  const importer::SourceProfile& source_profile =
      importer_list_->GetSourceProfileAt(index);
  uint16_t supported_items = source_profile.services_supported;
  uint16_t imported_items = (supported_items & importer::FAVORITES);
  if (imported_items) {
    if (importer::TYPE_BOOKMARKS_FILE == source_profile.importer_type) {
      HandleChooseBookmarksFile(source_profile, imported_items);
    }
    else
      ImportFromBrowser(source_profile, imported_items);
  } else {
    LOG(ERROR) << "There were no settings to import from '"
        << source_profile.importer_name << "'.";
  }

  selected_.erase(selected_.begin());
}

void ImportDialogView::ImportFromBrowser(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  selected_count_--;

  if (!imported_items)
    return ;

  if (importer_host_)
    importer_host_->set_observer(NULL);

  importer_host_ = new ExternalProcessImporterHost();
  DCHECK(importer_host_);
  if (!importer_host_)
    return ;

  importer_host_->set_observer(this);
  importer_host_->StartImportSettings(source_profile,
                                      ProfileManager::GetPrimaryUserProfile(),
                                      imported_items,
                                      new ProfileWriter(ProfileManager::GetPrimaryUserProfile()));

  importer::LogImporterUseToMetrics("BookmarksAPI",
                                    source_profile.importer_type);
}

void ImportDialogView::ButtonPressed(Button* sender, const ui::Event& event) {

}

// importer::ImporterProgressObserver
void ImportDialogView::ImportStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!start_) {
    RemoveAllChildViews(false);

    information_ = new Label();
    AddChildView(information_);
    information_->SetFontList(gfx::FontList("Courier, 25px"));
    information_->SetText(l10n_util::GetStringUTF16(IDS_CDOS_IMPORT_LOADING));
    information_->SetBounds(0, (height() - 50) / 2, width_, 30);
    information_->SetHorizontalAlignment(gfx::HorizontalAlignment::ALIGN_CENTER);


    const DialogClientView* dcv = static_cast<DialogClientView*>(dialog_client_view_);

    dcv->ok_button()->SetEnabled(false);
    dcv->cancel_button()->SetEnabled(false);

    start_ = true;
  }
}

void ImportDialogView::ImportItemStarted(ImportItem item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void ImportDialogView::ImportItemEnded(ImportItem item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (0 == selected_count_) {
    import_did_succeed_ = true;
    end_ = true;
  }
}

void ImportDialogView::ImportEnded() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (importer_host_) {
    importer_host_->set_observer(NULL);
    importer_host_ = NULL;
  }

  if (0 != selected_count_ && !import_did_succeed_) {
    BrowserThread::PostDelayedTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&ImportDialogView::StartImport,
                   base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(1000));
  }


  if (import_did_succeed_ && 0 == selected_count_) {
    // 导入成功后立即关闭导入对话框
    GetWidget()->Close();
  }

}

void ImportDialogView::FileSelected(const base::FilePath& path,
                                    int index,
                                    void* params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_BOOKMARKS_FILE;
  source_profile.source_path = path;

  ImportFromBrowser(source_profile, importer::FAVORITES);
}

void ImportDialogView::FileSelectionCanceled(void* params) {
  RemoveAllChildViews(false);
  Reset();
}

void ImportDialogView::HandleChooseBookmarksFile(const importer::SourceProfile& source_profile,
                                                 uint16_t imported_items) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, new ChromeSelectFilePolicy(
                    chrome::FindLastActiveWithProfile(profile_)->tab_strip_model()->GetActiveWebContents()));

  ui::SelectFileDialog::FileTypeInfo file_type_info;
  file_type_info.extensions.resize(1);
  file_type_info.extensions[0].push_back(FILE_PATH_LITERAL("html"));

  Browser* browser = chrome::FindLastActiveWithProfile(profile_);

  select_file_dialog_->SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE,
                                     base::string16(),
                                     base::FilePath(),
                                     &file_type_info,
                                     0,
                                     base::FilePath::StringType(),
                                     browser->window()->GetNativeWindow(),
                                     NULL);
}

void NfsLabelButton::OnMouseEntered(const ui::MouseEvent& event) {
  if (!selected_) {
    Background* background = Background::CreateSolidBackground(222, 222, 222);
    set_background(background);
    SchedulePaint();
  }
}

void NfsLabelButton::OnMouseExited(const ui::MouseEvent& event) {
  if (!selected_) {
    Background* background = Background::CreateSolidBackground(255, 255, 255);
    set_background(background);
    SchedulePaint();
  }
}

void NfsLabelButton::OnMouseReleased(const ui::MouseEvent& event) {
  Background* background = NULL;
  if (selected_) {
    selected_ = false;
    background = Background::CreateSolidBackground(255, 255, 255);
  }
  else {
    selected_ = true;
    background = Background::CreateSolidBackground(222, 236, 255);
  }

  set_background(background);
  SchedulePaint();
}
