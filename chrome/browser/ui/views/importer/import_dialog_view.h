#ifndef CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_DIALOG_VIEW_H_

#include <map>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/importer/importer_list.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/views/view.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class ButtonListener;
class Checkbox;
class ImageButton;
class Label;
class LabelButton;
class RadioButton;
}

class NfsLabelButton;
class ExternalProcessImporterHost;
class Profile;
class Browser;

struct BrowserInfo {
  NfsLabelButton* browser_image;
  base::string16 browser_name;
  importer::ImporterType browser_type;
  size_t index;
};

typedef std::map<base::string16, std::unique_ptr<BrowserInfo>> BrowserInfoMap;

void ShowImportSelectDialog(gfx::NativeWindow parent,
                            base::string16 title = base::string16(),
                            int width = 350);

// ImportDialogView asks the user to select Browser before starting the
// profile import.
class ImportDialogView : public views::NfsDialogDelegateView
                       , public importer::ImporterProgressObserver
                       , public views::ButtonListener
                       , public ui::SelectFileDialog::Listener
                       , public base::SupportsWeakPtr<ImportDialogView> {
 public:
  static void Show(gfx::NativeWindow parent, base::string16 title, int width);
  static void ShouldShowImportDialog(Browser* browser, Profile* profile);
  static void ShowImportFavoritesInfobar(Browser* browser, Profile* profile);

 private:
  ImportDialogView(base::string16 title);
  ImportDialogView(base::string16 title, int width);
  ~ImportDialogView() override;

  // views::View:
  gfx::Size GetPreferredSize() const override;
  void Layout() override;

  // views::DialogDelegate:
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  base::string16 GetWindowTitle() const override;
  bool Accept() override;
  bool Cancel() override;

  // importer::ImporterProgressObserver
  void ImportStarted() override;
  void ImportItemStarted(importer::ImportItem item) override;
  void ImportItemEnded(importer::ImportItem item) override;
  void ImportEnded() override;

  // ui::SelectFileDialog::Listener:
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectionCanceled(void* params) override;

  // Opens a file selection dialog to choose the bookmarks HTML file.
  void HandleChooseBookmarksFile(const importer::SourceProfile& source_profile,
                                 uint16_t imported_items);

 private:
  void Reset();

  void StartImport();
  void ImportFromBrowser(const importer::SourceProfile& source_profile,
                         uint16_t imported_items);

  void GetSupportedBrowser();
  void RemoveBookmarks();

  // ButtonListener
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  unsigned selected_count_;

  Profile* profile_;

  views::Checkbox* clear_;

  views::Label* information_;

  // If non-null it means importing is in progress. ImporterHost takes care
  // of deleting itself when import is complete.
  ExternalProcessImporterHost* importer_host_;  // weak

  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  BrowserInfoMap browser_info_list_;
  std::unique_ptr<ImporterList> importer_list_;

  std::vector<size_t> selected_;

  base::string16 title_;
  int width_;

  bool import_did_succeed_;
  bool start_;
  bool end_;

  base::WeakPtrFactory<ImportDialogView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImportDialogView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_IMPORTER_IMPORT_DIALOG_VIEW_H_
