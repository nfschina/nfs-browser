// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bookmarks/nfsbrowser_bookmark_bubble_view.h"

#include <string>

//#include "base/basictypes.h"
#include "base/logging.h"
//#include "base/prefs/pref_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils_desktop.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/url_formatter/url_fixer.h"
#include "grit/components_strings.h"
#include "grit/generated_resources.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/tree/tree_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/layout/layout_constants.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/widget/widget.h"
#include "ui/aura/window.h"
#include "ui/views/border.h"
#include "ui/strings/grit/ui_strings.h"
#include "url/gurl.h"

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
using bookmarks::BookmarkExpandedStateTracker;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using views::GridLayout;
using views::ColumnSet;
using views::Border;
using views::Label;
using views::Textfield;

namespace {
const int kBookmarkBubbleViewWidth = 400;
const int kBookmarkBubbleViewHeight = 276;
const int kBookmarkLocationLabelWidth = kBookmarkBubbleViewWidth / 2;
const int kButtonHEdgeMarginNew = 20;
const int kBubbleTopMargin = 10;
const int kBubbleBottomMargin = 20;
const int kUnrelatedControlHorizontalSpacing = 10;
const int kRelatedButtonHSpacing = 8;
const int kLocationRowTopMargin = 10;
const int kUnrelatedControlVerticalSpacing = 12;
const int kBookmarkedBubbleHeaderHeight = 40;
const int kBookmarkedLocationLabelTopMargin = 8;
const int kBookmarkedBubbleSeparatorColor = SkColorSetRGB(0x99, 0x99, 0x99);
const int kBookmarkedBubbleLocationLabelColor = SkColorSetRGB(0x99, 0x99, 0x99);

// Width of the border of a button.
const int kControlBorderWidth = 2;

// Background color of text field when URL is invalid.
const SkColor kErrorColor = SkColorSetRGB(0xFF, 0xBC, 0xBC);

}  // namespace

BookmarkBubbleView* BookmarkBubbleView::bookmark_bubble_ = NULL;

BookmarkBubbleView::BookmarkBubbleView(
    views::View* anchor_view,
    bookmarks::BookmarkBubbleObserver* observer,
    std::unique_ptr<BubbleSyncPromoDelegate> delegate,
    Profile* profile,
    const GURL& url,
    const base::string16 title,
    bool newly_bookmarked)
    : BubbleDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
      profile_(profile),
      bb_model_(BookmarkModelFactory::GetForBrowserContext(profile)),
      running_menu_for_root_(false),
      observer_(observer),
      url_(url),
      title_(title),
      newly_bookmarked_(newly_bookmarked),
      title_tf_(NULL),
      tree_view_(NULL),
      new_folder_button_(NULL),
      ok_button_(NULL),
      cancel_button_(NULL),
      edit_button_(NULL) ,
      remove_button_(NULL),
      remove_bookmark_(false),
      apply_edits_(newly_bookmarked) {
    DCHECK(profile);
    DCHECK(bb_model_);
    //DCHECK(bb_model_->client()->CanBeEditedByUser(parent));
    set_margins(gfx::Insets(kBubbleTopMargin, 0, 0, 0));
    // Compensate for built-in vertical padding in the anchor view's image.
    set_anchor_view_insets(gfx::Insets(0, 0, 0, 0));
}

BookmarkBubbleView::~BookmarkBubbleView() {
  if (apply_edits_) {
    ApplyEdits();
  } else if (remove_bookmark_) {
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile_);
    const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url_);
    if (node) {
      model->Remove(node);
    }
  }

  // The tree model is deleted before the view. Reset the model otherwise the
  // tree will reference a deleted model.
  if (tree_view_)
    tree_view_->SetModel(NULL);
  bb_model_->RemoveObserver(this);
}


gfx::Size BookmarkBubbleView::GetPreferredSize() const {
  return gfx::Size(kBookmarkBubbleViewWidth, (newly_bookmarked_ ?
      kBookmarkBubbleViewHeight : views::BubbleDelegateView::GetPreferredSize().height()));
}

void BookmarkBubbleView::OnTreeViewSelectionChanged(
    views::TreeView* tree_view) {
}

bool BookmarkBubbleView::CanEdit(views::TreeView* tree_view,
                                 ui::TreeModelNode* node) {
  // Only allow editting of children of the bookmark bar node and other node.
  EditorNode* bb_node = tree_model_->AsNode(node);
  return (bb_node->parent() && bb_node->parent()->parent());
}

void BookmarkBubbleView::ContentsChanged(views::Textfield* sender,
                                         const base::string16& new_contents) {
  UserInputChanged();
}

bool BookmarkBubbleView::HandleKeyEvent(views::Textfield* sender,
                                        const ui::KeyEvent& key_event) {
    return false;
}

void BookmarkBubbleView::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  if (sender == new_folder_button_) {
    NewFolder();
  } else if (sender == ok_button_) {
    apply_edits_ = true;
    GetWidget()->Close();
  } else if (sender == cancel_button_) {
    apply_edits_ = false;
    GetWidget()->Close();
  }  else if (sender == edit_button_) {
    apply_edits_ = false;
    ShowEditor();
  } else if (sender == remove_button_) {
    // Set this so we remove the bookmark after the window closes.
    remove_bookmark_ = true;
    apply_edits_ = false;
    GetWidget()->Close();
  }
}

void BookmarkBubbleView::HandleButtonPressed(views::Button* sender) {
  if (sender == new_folder_button_) {
    NewFolder();
  } else if (sender == ok_button_) {
    ApplyEdits();
    GetWidget()->Close();
  } else {
    DCHECK_EQ(cancel_button_, sender);
    GetWidget()->Close();
  }
}

bool BookmarkBubbleView::IsCommandIdChecked(int command_id) const {
  return false;
}

bool BookmarkBubbleView::IsCommandIdEnabled(int command_id) const {
  switch (command_id) {
    case IDS_EDIT:
    case IDS_DELETE:
      return !running_menu_for_root_;
    case IDS_BOOKMARK_EDITOR_NEW_FOLDER_MENU_ITEM:
      return true;
    default:
      NOTREACHED();
      return false;
  }
}

bool BookmarkBubbleView::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) const {
  return GetWidget()->GetAccelerator(command_id, accelerator);
}

void BookmarkBubbleView::ExecuteCommand(int command_id, int event_flags) {
  DCHECK(tree_view_->GetSelectedNode());
  if (command_id == IDS_EDIT) {
    tree_view_->StartEditing(tree_view_->GetSelectedNode());
  } else {
    DCHECK_EQ(IDS_BOOKMARK_EDITOR_NEW_FOLDER_MENU_ITEM, command_id);
    NewFolder();
  }
}

void BookmarkBubbleView::ShowContextMenuForView(
    views::View* source,
    const gfx::Point& point,
    ui::MenuSourceType source_type) {
  DCHECK_EQ(tree_view_, source);
  if (!tree_view_->GetSelectedNode())
    return;
  running_menu_for_root_ =
      (tree_model_->GetParent(tree_view_->GetSelectedNode()) ==
       tree_model_->GetRoot());

  context_menu_runner_.reset(new views::MenuRunner(
      GetMenuModel(),
      views::MenuRunner::HAS_MNEMONICS | views::MenuRunner::CONTEXT_MENU));

  if (context_menu_runner_->RunMenuAt(source->GetWidget()->GetTopLevelWidget(),
                                      NULL,
                                      gfx::Rect(point, gfx::Size()),
                                      views::MENU_ANCHOR_TOPRIGHT,
                                      source_type) ==
      views::MenuRunner::MENU_DELETED) {
    return;
  }
}


void BookmarkBubbleView::BookmarkNodeMoved(BookmarkModel* model,
                                           const BookmarkNode* old_parent,
                                           int old_index,
                                           const BookmarkNode* new_parent,
                                           int new_index) {
  Reset();
}

void BookmarkBubbleView::BookmarkNodeAdded(BookmarkModel* model,
                                           const BookmarkNode* parent,
                                           int index) {
  Reset();
}

void BookmarkBubbleView::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int index,
    const BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
    Reset();
}

void BookmarkBubbleView::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  Reset();
}

void BookmarkBubbleView::BookmarkNodeChildrenReordered(
    BookmarkModel* model,
    const BookmarkNode* node) {
  Reset();
}

void BookmarkBubbleView::Init() {
  bb_model_->AddObserver(this);

  views::Label* title_label = new views::Label(
    l10n_util::GetStringUTF16(
        newly_bookmarked_ ? IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARK_CDOS :
                            IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED_CDOS));
  ui::ResourceBundle* rb = &ui::ResourceBundle::GetSharedInstance();
  title_label->SetFontList(rb->GetFontList(ui::ResourceBundle::MediumFont));

  if (newly_bookmarked_) {
    new_folder_button_ = new views::LabelButton(this, l10n_util::GetStringUTF16(
                                                        IDS_BOOKMARK_EDITOR_NEW_FOLDER_BUTTON));
    new_folder_button_->SetStyle(views::Button::STYLE_BUTTON);

    ok_button_ = views::MdTextButton::CreateSecondaryUiBlueButton(
        this, l10n_util::GetStringUTF16(IDS_APP_OK));
    ok_button_->SetIsDefault(true);

    cancel_button_ = new views::LabelButton(
        this, l10n_util::GetStringUTF16(IDS_APP_CANCEL));
    cancel_button_->SetStyle(views::Button::STYLE_BUTTON);

    views::Label* location_label = new views::Label(
        l10n_util::GetStringUTF16(IDS_PLUGINS_PATH));

    tree_view_ = new views::TreeView();
    tree_view_->SetRootShown(false);
    tree_view_->set_context_menu_controller(this);

    GridLayout* layout = new GridLayout(this);
    SetLayoutManager(layout);

    // Column sets used in the layout of the bubble.
    enum ColumnSetID {
      TITLE_COLUMN_SET_ID,
      BODY_COLUMN_SET_ID,
      BUTTON_COLUMN_SET_ID,
    };

    // The column layout used for title rows.
    ColumnSet* cs = layout->AddColumnSet(TITLE_COLUMN_SET_ID);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
    cs->AddColumn(GridLayout::CENTER, GridLayout::CENTER, 0, GridLayout::USE_PREF,
                  0, 0);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);

    //The column layout used for body rows.
    cs = layout->AddColumnSet(BODY_COLUMN_SET_ID);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
    cs->AddColumn(GridLayout::CENTER, GridLayout::CENTER, 0, GridLayout::USE_PREF,
                  0, 0);
    cs->AddPaddingColumn(0, kUnrelatedControlHorizontalSpacing);
    cs->AddColumn(GridLayout::FILL, GridLayout::FILL, 1, GridLayout::USE_PREF,
                  0, 0);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);

    // The column layout used for button rows.
    cs = layout->AddColumnSet(BUTTON_COLUMN_SET_ID);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
    cs->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 0,
                  GridLayout::USE_PREF, 0, 0);
    cs->AddPaddingColumn(0, views::kUnrelatedControlLargeHorizontalSpacing);

    cs->AddColumn(GridLayout::TRAILING, GridLayout::CENTER, 1,
                  GridLayout::USE_PREF, 0, 0);
    cs->AddPaddingColumn(0, kRelatedButtonHSpacing);

    cs->AddColumn(GridLayout::TRAILING, GridLayout::CENTER, 0,
                  GridLayout::USE_PREF, 0, 0);
    cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);

    //Add title row
    layout->StartRow(0, TITLE_COLUMN_SET_ID);
    layout->AddView(title_label);
    layout->AddPaddingRow(0, kUnrelatedControlVerticalSpacing);

    //Add name row
    layout->StartRow(0, BODY_COLUMN_SET_ID);
    views::Label* label = new views::Label(
        l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_TITLE_TEXT));
    layout->AddView(label);
    title_tf_ = new views::Textfield();
    title_tf_->SetText(GetTitle());
    title_tf_->SetAccessibleName(
        l10n_util::GetStringUTF16(IDS_BOOKMARK_AX_BUBBLE_TITLE_TEXT));
    layout->AddView(title_tf_);
    layout->AddPaddingRow(0, kLocationRowTopMargin);

    //Add location row
    layout->StartRow(1, BODY_COLUMN_SET_ID);
    layout->AddView(location_label, 1, 1, GridLayout::CENTER, GridLayout::LEADING);
    layout->AddView(tree_view_->CreateParentIfNecessary());
    layout->AddPaddingRow(0, kUnrelatedControlVerticalSpacing);

    //Add button row
    layout->StartRow(0, BUTTON_COLUMN_SET_ID);
    layout->AddView(new_folder_button_);
    layout->AddView(ok_button_);
    layout->AddView(cancel_button_);
    layout->AddPaddingRow(0, kUnrelatedControlVerticalSpacing - kControlBorderWidth);

    AddAccelerator(ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE));
    AddAccelerator(ui::Accelerator(ui::VKEY_E, ui::EF_ALT_DOWN));
    AddAccelerator(ui::Accelerator(ui::VKEY_R, ui::EF_ALT_DOWN));

    if (bb_model_->loaded()) {
      Reset(true);
    }
  } else {   // !newly_bookmarked_
       // Column sets used in the layout of the bubble.
      enum ColumnSetID {
        TITLE_COLUMN_SET_ID,
        SEPARATOR_COLUMN_SET_ID,
        BODY_COLUMN_SET_ID,
      };

      GridLayout* layout = new GridLayout(this);
      SetLayoutManager(layout);

       // The column layout used for title rows.
      ColumnSet* cs = layout->AddColumnSet(TITLE_COLUMN_SET_ID);
      cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
      cs->AddColumn(GridLayout::LEADING, GridLayout::LEADING, 0, GridLayout::USE_PREF,
                    0, 0);

      // The column layout used for separator rows.
      cs = layout->AddColumnSet(SEPARATOR_COLUMN_SET_ID);
      cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
      cs->AddColumn(GridLayout::FILL, GridLayout::CENTER, 1, GridLayout::USE_PREF,
                    0, 0);
      cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);

       // The column layout used for body rows.
      cs = layout->AddColumnSet(BODY_COLUMN_SET_ID);
      cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);
      cs->AddColumn(GridLayout::LEADING, GridLayout::TRAILING, 0, GridLayout::FIXED,
                    kBookmarkLocationLabelWidth, 0);
      cs->AddPaddingColumn(1, views::kUnrelatedControlLargeHorizontalSpacing);
      cs->AddColumn(GridLayout::TRAILING, GridLayout::TRAILING, 1, GridLayout::USE_PREF,
                    0, 0);
      cs->AddPaddingColumn(0, kRelatedButtonHSpacing);
      cs->AddColumn(GridLayout::TRAILING, GridLayout::TRAILING, 0, GridLayout::USE_PREF,
                    0, 0);
      cs->AddPaddingColumn(0, kButtonHEdgeMarginNew);

      //Add title row
      layout->StartRow(0, TITLE_COLUMN_SET_ID);
      layout->AddView(title_label, 1, 1, GridLayout::LEADING, GridLayout::LEADING, 0, kBookmarkedBubbleHeaderHeight - kBubbleTopMargin);

      //Add separator row
      layout->StartRow(0, SEPARATOR_COLUMN_SET_ID);
      views::Separator* separator = new views::Separator(views::Separator::HORIZONTAL);
            separator->SetColor((SkColor)kBookmarkedBubbleSeparatorColor);
      layout->AddView(separator);
      layout->AddPaddingRow(0, kUnrelatedControlVerticalSpacing);

      //Add name row
      layout->StartRow(0, BODY_COLUMN_SET_ID);
      views::Label* label = new views::Label();
      const BookmarkNode* node = bb_model_->GetMostRecentlyAddedUserNodeForURL(url_);
      label->SetText(node->GetTitle());
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      label->SetElideBehavior(gfx::ELIDE_TAIL);
      layout->AddView(label);
      edit_button_ = new views::LabelButton(
                this, l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_OPTIONS));
      edit_button_->SetStyle(views::Button::STYLE_BUTTON);
      layout->AddView(edit_button_, 1, 3);
      remove_button_ = new views::LabelButton(
                this, l10n_util::GetStringUTF16(IDS_BOOKMARK_BUBBLE_REMOVE_BOOKMARK));
      remove_button_->SetStyle(views::Button::STYLE_BUTTON);
      layout->AddView(remove_button_, 1, 3);
      layout->AddPaddingRow(0, kBookmarkedLocationLabelTopMargin);

      //Add location row
      layout->StartRow(0, BODY_COLUMN_SET_ID);
      views::Label* location_label = new views::Label();
      location_label->SetText(GetBookmarkedLocation(url_));
      label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
      location_label->SetElideBehavior(gfx::ELIDE_TAIL);
      location_label->SetEnabledColor((SkColor)kBookmarkedBubbleLocationLabelColor);
      layout->AddView(location_label);
      layout->AddPaddingRow(0, kBubbleBottomMargin - kControlBorderWidth);
  }
}

void BookmarkBubbleView::Reset(bool is_add) {
  if (!bookmark_bubble_) {
    return;
  }

  new_folder_button_->SetEnabled(true);

  // Do this first, otherwise when we invoke SetModel with the real one
  // tree_view will try to invoke something on the model we just deleted.
  tree_view_->SetModel(NULL);
  tree_model_.reset(new EditorTreeModel(CreateRootNode()));

  tree_view_->SetModel(tree_model_.get());
  tree_view_->SetController(this);

  context_menu_runner_.reset();

  if (parent()) {
    ExpandAndSelect();
  } else if (is_add) {
    const BookmarkNode* node = bb_model_->GetParentForNewNodes();
    if (!node) {
      return;
    }

    EditorNode* b_node = tree_model_->GetRoot()->GetChild(0);
    tree_view_->ExpandAll(b_node);

    EditorNode* editor_node =
        FindNodeWithID(tree_model_->GetRoot(), node->id());
    if (editor_node) {
      tree_view_->Expand(editor_node);
      tree_view_->SetSelectedNode(editor_node);
    }
  }
}

GURL BookmarkBubbleView::GetInputURL() const {
  return url_;
}

void BookmarkBubbleView::UserInputChanged() {
  const GURL url(GetInputURL());
  if (!url.is_valid()) {
    title_tf_->SetBackgroundColor(kErrorColor);
  } else {
    title_tf_->UseDefaultBackgroundColor();
  }
  //GetDialogClientView()->UpdateDialogButtons();
}

void BookmarkBubbleView::NewFolder() {
  // Create a new entry parented to the selected item, or the bookmark
  // bar if nothing is selected.
  EditorNode* parent = tree_model_->AsNode(tree_view_->GetSelectedNode());
  if (!parent) {
    NOTREACHED();
    return;
  }

  tree_view_->StartEditing(AddNewFolder(parent));
}

BookmarkBubbleView::EditorNode* BookmarkBubbleView::AddNewFolder(
    EditorNode* parent) {
  return tree_model_->Add(parent, base::MakeUnique<EditorNode>(
        l10n_util::GetStringUTF16(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME), 0),
         parent->child_count());
}

void BookmarkBubbleView::ExpandAndSelect() {
  BookmarkExpandedStateTracker::Nodes expanded_nodes =
      bb_model_->expanded_state_tracker()->GetExpandedNodes();
  for (BookmarkExpandedStateTracker::Nodes::const_iterator i(
       expanded_nodes.begin()); i != expanded_nodes.end(); ++i) {
    EditorNode* editor_node =
        FindNodeWithID(tree_model_->GetRoot(), (*i)->id());
    if (editor_node)
      tree_view_->Expand(editor_node);
  }

  EditorNode*  b_node = tree_model_->GetRoot()->GetChild(0);  // Bookmark bar node.
  tree_view_->SetSelectedNode(b_node);
}

std::unique_ptr<BookmarkBubbleView::EditorNode> BookmarkBubbleView::CreateRootNode() {
  std::unique_ptr<EditorNode> root_node =
      base::MakeUnique<EditorNode>(base::string16(), 0);
  const BookmarkNode* bb_root_node = bb_model_->root_node();
  CreateNodes(bb_root_node, root_node.get());
  //There is only one node(bookmark bar) in current nfsbrowser.
  DCHECK(root_node->child_count() == 1);
  DCHECK_EQ(BookmarkNode::BOOKMARK_BAR, bb_root_node->GetChild(0)->type());

  return root_node;
}

void BookmarkBubbleView::CreateNodes(const BookmarkNode* bb_node,
                                     BookmarkBubbleView::EditorNode* b_node) {
  for (int i = 0; i < bb_node->child_count(); ++i) {
    const BookmarkNode* child_bb_node = bb_node->GetChild(i);
    if (child_bb_node->IsVisible() && child_bb_node->is_folder() &&
        bb_model_->client()->CanBeEditedByUser(child_bb_node)) {
      EditorNode* new_b_node =
            b_node->Add(base::MakeUnique<EditorNode>(child_bb_node->GetTitle(),
                                      child_bb_node->id()), b_node->child_count());
      CreateNodes(child_bb_node, new_b_node);
    }
  }
}

BookmarkBubbleView::EditorNode* BookmarkBubbleView::FindNodeWithID(
    BookmarkBubbleView::EditorNode* node,
    int64_t id) {
  if (node->value == id)
    return node;
  for (int i = 0; i < node->child_count(); ++i) {
    EditorNode* result = FindNodeWithID(node->GetChild(i), id);
    if (result)
      return result;
  }
  return NULL;
}

void BookmarkBubbleView::ApplyEdits() {
  DCHECK(bb_model_->loaded());

  if (tree_view_)
    tree_view_->CommitEdit();

  EditorNode* parent = tree_model_->AsNode(tree_view_->GetSelectedNode());
  if (!parent) {
    NOTREACHED();
    return;
  }
  ApplyEdits(parent);
}

void BookmarkBubbleView::ApplyEdits(EditorNode* parent) {
  DCHECK(parent);

  // We're going to apply edits to the bookmark bar model, which will call us
  // back. Normally when a structural edit occurs we reset the tree model.
  // We don't want to do that here, so we remove ourselves as an observer.
  bb_model_->RemoveObserver(this);
  GURL new_url(GetInputURL());
  base::string16 new_title(title_tf_->text());

  // Create the new folders and update the titles.
  const BookmarkNode* new_parent = NULL;
  ApplyNameChangesAndCreateNewFolders(
      bb_model_->root_node(), tree_model_->GetRoot(), parent, &new_parent);

  const BookmarkNode* bb_parent = bookmarks::GetBookmarkNodeByID(bb_model_, parent->value);
  int index = bb_parent->child_count();
  BookmarkEditor::ApplyEditsWithPossibleFolderChange(
      bb_model_, new_parent, BookmarkEditor::EditDetails::AddNodeInFolder(
                      bb_parent, index, new_url, new_title), new_title, new_url);

  BookmarkExpandedStateTracker::Nodes expanded_nodes;
  UpdateExpandedNodes(tree_model_->GetRoot(), &expanded_nodes);
  bb_model_->expanded_state_tracker()->SetExpandedNodes(expanded_nodes);
}

void BookmarkBubbleView::ApplyNameChangesAndCreateNewFolders(
    const BookmarkNode* bb_node,
    BookmarkBubbleView::EditorNode* b_node,
    BookmarkBubbleView::EditorNode* parent_b_node,
    const BookmarkNode** parent_bb_node) {
  if (parent_b_node == b_node)
    *parent_bb_node = bb_node;
  for (int i = 0; i < b_node->child_count(); ++i) {
    EditorNode* child_b_node = b_node->GetChild(i);
    const BookmarkNode* child_bb_node = NULL;
    if (child_b_node->value == 0) {
      // New folder.
      child_bb_node = bb_model_->AddFolder(bb_node,
          bb_node->child_count(), child_b_node->GetTitle());
      child_b_node->value = child_bb_node->id();
    } else {
      // Existing node, reset the title (BookmarkModel ignores changes if the
      // title is the same).
      for (int j = 0; j < bb_node->child_count(); ++j) {
        const BookmarkNode* node = bb_node->GetChild(j);
        if (node->is_folder() && node->id() == child_b_node->value) {
          child_bb_node = node;
          break;
        }
      }
      DCHECK(child_bb_node);
      bb_model_->SetTitle(child_bb_node, child_b_node->GetTitle());
    }
    ApplyNameChangesAndCreateNewFolders(child_bb_node, child_b_node,
                                        parent_b_node, parent_bb_node);
  }
}

void BookmarkBubbleView::UpdateExpandedNodes(
    EditorNode* editor_node,
    BookmarkExpandedStateTracker::Nodes* expanded_nodes) {
  if (!tree_view_->IsExpanded(editor_node))
    return;

  // The root is 0.
  if (editor_node->value != 0) {
    expanded_nodes->insert(
        bookmarks::GetBookmarkNodeByID(bb_model_, editor_node->value));
  }

  for (int i = 0; i < editor_node->child_count(); ++i)
    UpdateExpandedNodes(editor_node->GetChild(i), expanded_nodes);
}

base::string16 BookmarkBubbleView::GetBookmarkedLocation(GURL url) {
  const BookmarkNode* node = bb_model_->GetMostRecentlyAddedUserNodeForURL(url);
  base::string16 location;
  TraverseNode(location, node);
  location = location.erase(0, 1);  //strip first "/"
  location.insert(0, l10n_util::GetStringUTF16(IDS_PLUGINS_PATH));
  return location;
}

void BookmarkBubbleView::TraverseNode(base::string16& location, const bookmarks::BookmarkNode* node) {
  const BookmarkNode* parent = node->parent();
  if (parent) {
    if (!parent->is_root()) {
      TraverseNode(location, parent);
      location.append(base::ASCIIToUTF16("/"));
    }
    location.append(parent->GetTitle());
  }
}

void BookmarkBubbleView::ShowEditor() {
  const BookmarkNode* node = bb_model_->GetMostRecentlyAddedUserNodeForURL(url_);
  gfx::NativeWindow native_parent =
      anchor_widget() ? anchor_widget()->GetNativeWindow()
                      : platform_util::GetTopLevel(parent_window());
  DCHECK(native_parent);

  Profile* profile = profile_;
  GetWidget()->Close();

  if (node && native_parent)
    BookmarkEditor::Show(native_parent, profile,
                         BookmarkEditor::EditDetails::EditNode(node),
                         BookmarkEditor::SHOW_TREE);
}

ui::SimpleMenuModel* BookmarkBubbleView::GetMenuModel() {
  if (!context_menu_model_.get()) {
    context_menu_model_.reset(new ui::SimpleMenuModel(this));
    context_menu_model_->AddItemWithStringId(IDS_EDIT, IDS_EDIT);
    // context_menu_model_->AddItemWithStringId(IDS_DELETE, IDS_DELETE);
    context_menu_model_->AddItemWithStringId(
        IDS_BOOKMARK_EDITOR_NEW_FOLDER_MENU_ITEM,
        IDS_BOOKMARK_EDITOR_NEW_FOLDER_MENU_ITEM);
  }
  return context_menu_model_.get();
}

void BookmarkBubbleView::EditorTreeModel::SetTitle(
    ui::TreeModelNode* node,
    const base::string16& title) {
  if (!title.empty())
    ui::TreeNodeModel<EditorNode>::SetTitle(node, title);
}

// static
views::Widget* BookmarkBubbleView::ShowBubble(views::View* anchor_view,
                                    const gfx::Rect& anchor_rect,
                                    gfx::NativeView parent_window,
                                    bookmarks::BookmarkBubbleObserver* observer,
                                    std::unique_ptr<BubbleSyncPromoDelegate> delegate,
                                    Profile* profile,
                                    const GURL& url,
                                    const base::string16 title,
                                    bool already_bookmarked) {
  if (bookmark_bubble_)
    return NULL;

  bookmark_bubble_ =
      new BookmarkBubbleView(anchor_view, observer, std::move(delegate), profile,
                             url, title, !already_bookmarked);
  if (!anchor_view) {
    bookmark_bubble_->SetAnchorRect(anchor_rect);
    bookmark_bubble_->set_parent_window(parent_window);
  }
  views::BubbleDelegateView::CreateBubble(bookmark_bubble_)->Show();
  // Select the entire title textfield contents when the bubble is first shown.
  if (!already_bookmarked) {
    bookmark_bubble_->title_tf_->SelectAll(true);
  }
  //bookmark_bubble_->SetArrowPaintType(views::BubbleBorder::PAINT_NONE);

  if (bookmark_bubble_->observer_) {
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
    const BookmarkNode* node = model->GetMostRecentlyAddedUserNodeForURL(url);
    bookmark_bubble_->observer_->OnBookmarkBubbleShown(node);
  }

  return bookmark_bubble_->GetWidget();
}

void BookmarkBubbleView::Hide() {
  if (bookmark_bubble_)
    bookmark_bubble_->GetWidget()->Close();
}

void BookmarkBubbleView::WindowClosing() {
  // We have to reset |bubble_| here, not in our destructor, because we'll be
  // destroyed asynchronously and the shown state will be checked before then.
  DCHECK_EQ(bookmark_bubble_, this);
  bookmark_bubble_ = NULL;

  if (observer_)
    observer_->OnBookmarkBubbleHidden();
}

bool BookmarkBubbleView::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  ui::KeyboardCode key_code = accelerator.key_code();
  if (key_code == ui::VKEY_RETURN) {
    HandleButtonPressed(ok_button_);
    return true;
  }
  if (key_code == ui::VKEY_E && accelerator.IsAltDown()) {
    HandleButtonPressed(ok_button_);
    return true;
  }
  if (key_code == ui::VKEY_R && accelerator.IsAltDown()) {
    HandleButtonPressed(new_folder_button_);
    return true;
  }
  if (key_code == ui::VKEY_ESCAPE) {
  }

  return BubbleDelegateView::AcceleratorPressed(accelerator);
}

const char* BookmarkBubbleView::GetClassName() const {
  return "BookmarkBubbleView";
}

// views::View* BookmarkBubbleView::GetInitiallyFocusedView() {
//   return title_tf_;
// }

base::string16 BookmarkBubbleView::GetTitle() {
  return title_;
}

void BookmarkBubbleView::GetAccessibleState(ui::AXViewState* state) {
  BubbleDelegateView::GetAccessibleState(state);
  state->name =
      l10n_util::GetStringUTF16(
          newly_bookmarked_ ? IDS_BOOKMARK_BUBBLE_PAGE_BOOKMARKED :
                              IDS_BOOKMARK_AX_BUBBLE_PAGE_BOOKMARK);
}
