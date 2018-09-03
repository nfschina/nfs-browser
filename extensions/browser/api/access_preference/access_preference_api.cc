#include "extensions/browser/api/access_preference/access_preference_api.h"

#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace extensions {

namespace accessPreference = api::access_preference;

AccessPreferenceGetBooleanFunction::AccessPreferenceGetBooleanFunction() {}

ExtensionFunction::ResponseAction AccessPreferenceGetBooleanFunction::Run() {
  std::unique_ptr<accessPreference::GetBoolean::Params> params(
  accessPreference::GetBoolean::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = GetProfile();
  if (profile) {
    bool result = profile->GetPrefs()->GetBoolean(params->key);
    std::unique_ptr<base::ListValue> output =
    accessPreference::GetBoolean::Results::Create(result);
    return RespondNow(ArgumentList(std::move(output)));
  }
  return RespondNow(NoArguments());
}

}//end extensions
