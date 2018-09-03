#ifndef CHROME_COMMON_CHROME_UTIL_H_
#define CHROME_COMMON_CHROME_UTIL_H_

class GURL;
class Profile;

namespace chrome {

  bool IsSetHomePage(Profile* profile, GURL* url);

} // namespace chrome

#endif  // CHROME_COMMON_CHROME_UTIL_H_
