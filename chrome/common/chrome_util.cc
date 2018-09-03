#include "chrome/common/chrome_util.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace chrome {
  bool IsSetHomePage (Profile* profile, GURL* url) {
    DCHECK(profile);

    if (!profile)
      return false;

    if (profile->GetPrefs()->GetInteger(prefs::kNewTabPage) != 1)
      return false;

    std::string url_string;
    url_string = profile->GetPrefs()->GetString(prefs::kHomePage);

    if (url_string.empty())
      return false;

    GURL home_page_url(url_string);
    *url = home_page_url;
    if (!url->is_valid())
      return false;

    return true;
  }
}
