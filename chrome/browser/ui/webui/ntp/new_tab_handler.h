#ifndef CHROME_BROWSER_UI_WEBUI_NTP_NEW_TAB_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_NEW_TAB_HANDLER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/thumbnails/thumbnailing_context.h"
#include "chrome/browser/ui/webui/ntp/thumbnail_dialog.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/search_engines/template_url_service_observer.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;
class TemplateURLService;
struct TemplateURLData;

namespace history {
  class TopSites;
}

namespace thumbnails {
  class ThumbnailService;
}

class NewTabHandler : public content::WebUIMessageHandler
                    , public TemplateURLServiceObserver
                    , public history::TopSitesObserver {
public:
  NewTabHandler(Profile* profile);
  ~NewTabHandler() override;

  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // TemplateURLServiceObserver implemention.
  void OnTemplateURLServiceChanged() override;

  // TopSitesObserver implemention.
  void TopSitesLoaded(history::TopSites* top_sites) override;
  void TopSitesChanged(history::TopSites* top_sites,
                       history::TopSitesObserver::ChangeReason change_reason) override;

private:
  void HandleDBIsLoaded(const base::ListValue* args);
  void HandleGetBookmarkNodeByID(const base::ListValue* args);
  void HandleGetDefaultSearchEngine(const base::ListValue* args);
  void HandleGetDefaultWebsites(const base::ListValue* args);
  void HandleGetRecommendedWebsites(const base::ListValue* args);
  void HandleGetSearchEngines(const base::ListValue* args);
  void HandleGetThumbnail(const base::ListValue* args);
  void HandleGetTopSites(const base::ListValue* args);
  void HandleOpenURL(const base::ListValue* args);
  void HandleSetSearchEngine(const base::ListValue* args);
  void HandleStartSearch(const base::ListValue* args);
  void OnMostVisitedURLsAvailable(const history::MostVisitedURLList& data);

  void FetchThumbnail(const ThumbnailDialog::FetchThumbnailResult result,
                      const int index, const GURL& url);

  Profile* profile_;

  scoped_refptr<thumbnails::ThumbnailService> thumbnail_service_;
  scoped_refptr<history::TopSites> top_sites_;
  std::unique_ptr<TemplateURLData> previous_default_search_provider_;
  TemplateURLService* template_url_service_;

  scoped_refptr<thumbnails::ThumbnailingContext> thumbnailing_context_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<NewTabHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NewTabHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_NEW_TAB_HANDLER_H_
