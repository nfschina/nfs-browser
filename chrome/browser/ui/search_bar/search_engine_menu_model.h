// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_BAR_SEARCH_ENGINE_MENU_MODEL_H_
#define CHROME_BROWSER_UI_SEARCH_BAR_SEARCH_ENGINE_MENU_MODEL_H_

#include "components/search_engines/template_url_service_observer.h"
#include "components/search_engines/template_url.h"
#include "ui/base/models/simple_menu_model.h"

#include <vector>

class Profile;//yana add 160623
class TemplateURLService;

class SearchEngineMenuModelDelegate {
public:
  virtual void OnDefaultSearchEngineChange() = 0;

protected:
  ~SearchEngineMenuModelDelegate() {}
};

class SearchEngineMenuModel : public ui::SimpleMenuModel ,
								public ui::SimpleMenuModel::Delegate,
                                public TemplateURLServiceObserver {
 public:
  SearchEngineMenuModel(Profile* profile);
  ~SearchEngineMenuModel() override;

  void SetSearchIconViewFunc(SearchEngineMenuModelDelegate* delegate) { delegate_ = delegate; };

  void OnTemplateURLServiceChanged()  override;


 private:
  Profile* profile_;//yana add 160623
  void Build();
  // Overridden from ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  void CommandIdHighlighted(int command_id) override;
  void ExecuteCommand(int command_id, int event_flags) override;

  SearchEngineMenuModelDelegate* delegate_;
  std::vector<TemplateURL*> template_urls_;

  DISALLOW_COPY_AND_ASSIGN(SearchEngineMenuModel);
};
#endif  // CHROME_BROWSER_UI_SEARCH_BAR_SEARCH_ENGINE_MENU_MODEL_H_
