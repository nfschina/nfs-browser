#include "extensions/common/api/access_preference.h"
#include "chrome/browser/extensions/chrome_extension_function.h"

namespace extensions {
  class AccessPreferenceGetBooleanFunction : public ChromeUIThreadExtensionFunction {
    public:
      AccessPreferenceGetBooleanFunction();
    protected:
      ~AccessPreferenceGetBooleanFunction() override {}
    private:
      DECLARE_EXTENSION_FUNCTION("accessPreference.GetBoolean", ACCESS_PREFERENCE_GETBOOLEAN);
      ResponseAction Run() final;
  };
}
