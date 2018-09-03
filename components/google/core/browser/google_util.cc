// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/google/core/browser/google_util.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/google/core/browser/google_switches.h"
#include "components/google/core/browser/google_url_tracker.h"
#include "components/url_formatter/url_fixer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

// Only use Link Doctor on official builds.  It uses an API key, too, but
// seems best to just disable it, for more responsive error pages and to reduce
// server load.
#if defined(GOOGLE_CHROME_BUILD)
#define LINKDOCTOR_SERVER_REQUEST_URL "https://www.googleapis.com/rpc"
#else
#define LINKDOCTOR_SERVER_REQUEST_URL ""
#endif


// Helpers --------------------------------------------------------------------

namespace {

bool gUseMockLinkDoctorBaseURLForTesting = false;

bool IsPathHomePageBase(base::StringPiece path) {
  return (path == "/") || (path == "/webhp");
}

}  // namespace


namespace google_util {

// Global functions -----------------------------------------------------------

bool HasGoogleSearchQueryParam(base::StringPiece str) {
  return false;
}

GURL LinkDoctorBaseURL() {
  if (gUseMockLinkDoctorBaseURLForTesting)
    return GURL("http://mock.linkdoctor.url/for?testing");
  return GURL(LINKDOCTOR_SERVER_REQUEST_URL);
}

void SetMockLinkDoctorBaseURLForTesting() {
  gUseMockLinkDoctorBaseURLForTesting = true;
}

std::string GetGoogleLocale(const std::string& application_locale) {
  // Google does not recognize "nb" for Norwegian Bokmal; it uses "no".
  return (application_locale == "nb") ? "no" : application_locale;
}

GURL AppendGoogleLocaleParam(const GURL& url,
                             const std::string& application_locale) {
  return net::AppendQueryParameter(
      url, "hl", GetGoogleLocale(application_locale));
}

std::string GetGoogleCountryCode(const GURL& google_homepage_url) {
  base::StringPiece google_hostname = google_homepage_url.host_piece();
  const size_t last_dot = google_hostname.find_last_of('.');
  if (last_dot == std::string::npos) {
    NOTREACHED();
  }
  base::StringPiece country_code = google_hostname.substr(last_dot + 1);
  // Assume the com TLD implies the US.
  if (country_code == "com")
    return "us";
  // Google uses the Unicode Common Locale Data Repository (CLDR), and the CLDR
  // code for the UK is "gb".
  if (country_code == "uk")
    return "gb";
  // Catalonia does not have a CLDR country code, since it's a region in Spain,
  // so use Spain instead.
  if (country_code == "cat")
    return "es";
  return country_code.as_string();
}

GURL GetGoogleSearchURL(const GURL& google_homepage_url) {
  // To transform the homepage URL into the corresponding search URL, add the
  // "search" and the "q=" query string.
  GURL::Replacements replacements;
  replacements.SetPathStr("search");
  replacements.SetQueryStr("q=");
  return google_homepage_url.ReplaceComponents(replacements);
}

const GURL& CommandLineGoogleBaseURL() {
  // Unit tests may add command-line flags after the first call to this
  // function, so we don't simply initialize a static |base_url| directly and
  // then unconditionally return it.
  CR_DEFINE_STATIC_LOCAL(std::string, switch_value, ());
  CR_DEFINE_STATIC_LOCAL(GURL, base_url, ());
  std::string current_switch_value(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kGoogleBaseURL));
  if (current_switch_value != switch_value) {
    switch_value = current_switch_value;
    base_url = url_formatter::FixupURL(switch_value, std::string());
    if (!base_url.is_valid() || base_url.has_query() || base_url.has_ref())
      base_url = GURL();
  }
  return base_url;
}

bool StartsWithCommandLineGoogleBaseURL(const GURL& url) {
  const GURL& base_url(CommandLineGoogleBaseURL());
  return base_url.is_valid() &&
         base::StartsWith(url.possibly_invalid_spec(), base_url.spec(),
                          base::CompareCase::SENSITIVE);
}

bool IsGoogleHostname(base::StringPiece host,
                      SubdomainPermission subdomain_permission) {
  return false;
}

bool IsGoogleDomainUrl(const GURL& url,
                       SubdomainPermission subdomain_permission,
                       PortPermission port_permission) {
  return false;
}

bool IsGoogleHomePageUrl(const GURL& url) {
  // First check to see if this has a Google domain.
  if (!IsGoogleDomainUrl(url, DISALLOW_SUBDOMAIN, DISALLOW_NON_STANDARD_PORTS))
    return false;

  // Make sure the path is a known home page path.
  base::StringPiece path(url.path_piece());
  return IsPathHomePageBase(path) ||
         base::StartsWith(path, "/ig", base::CompareCase::INSENSITIVE_ASCII);
}

bool IsGoogleSearchUrl(const GURL& url) {
  // First check to see if this has a Google domain.
  if (!IsGoogleDomainUrl(url, DISALLOW_SUBDOMAIN, DISALLOW_NON_STANDARD_PORTS))
    return false;

  // Make sure the path is a known search path.
  base::StringPiece path(url.path_piece());
  bool is_home_page_base = IsPathHomePageBase(path);
  if (!is_home_page_base && (path != "/search"))
    return false;

  // Check for query parameter in URL parameter and hash fragment, depending on
  // the path type.
  return HasGoogleSearchQueryParam(url.ref_piece()) ||
      (!is_home_page_base && HasGoogleSearchQueryParam(url.query_piece()));
}

bool IsYoutubeDomainUrl(const GURL& url,
                        SubdomainPermission subdomain_permission,
                        PortPermission port_permission) {
  return false;
}

}  // namespace google_util
