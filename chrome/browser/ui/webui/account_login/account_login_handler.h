// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_HANDLER_H_

#include <string>

#include "base/debug/leak_tracker.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/signin/core/browser/account_info.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"

class NfsSyncService;

namespace net {
class URLFetcher;
}

class IOThread;
class Profile;

class MyURLRequestContextGetter : public net::URLRequestContextGetter {
public:
  explicit MyURLRequestContextGetter(IOThread* const io_thread);

  // Implementation for net::UrlRequestContextGetter.
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

private:
  ~MyURLRequestContextGetter() override;

  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;

  base::debug::LeakTracker<MyURLRequestContextGetter> leak_tracker_;
};

// Handles actions on Welcome page.
class AccountLoginHandler : public content::WebUIMessageHandler
                          , public net::URLFetcherDelegate {
public:
#if defined(OS_POSIX)
  // On most platforms, native pathnames are char arrays, and the encoding
  // may or may not be specified.  On Mac OS X, native pathnames are encoded
  // in UTF-8.
  typedef std::string StringType;
#elif defined(OS_WIN)
  // On Windows, for Unicode-aware applications, native pathnames are wchar_t
  // arrays encoded in UTF-16.
  typedef std::wstring StringType;
#endif  // OS_WIN

  explicit AccountLoginHandler(content::WebUI* web_ui);
  ~AccountLoginHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

private:
  void CheckIsLoggedIn(const base::ListValue* args);
  void HandleGetAccountId(const base::ListValue* args);
  void HandleLogin(const base::ListValue* args);
  void HandleLogout(const base::ListValue* args);
  void HandleGetEmail(const base::ListValue* args);
  void HandleGetOptions(const base::ListValue* args);
  void HandleChangeSyncBookmark(const base::ListValue* args);
  void HandleChangeSyncSkin(const base::ListValue* args);
  void HandleChangeSyncPassword(const base::ListValue* args);
  void HandleSyncNow(const base::ListValue* args);

  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void ReportLoginResult(int error_code);

  std::string GetAccountID();
  std::string GetAccountEmail();
  void SaveToPref(AccountInfo info);

  Profile* profile_;

  NfsSyncService* nfs_sync_service_;

  std::unique_ptr<net::URLFetcher> fetcher_;

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<AccountLoginHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AccountLoginHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_ACCOUNT_LOGIN_ACCOUNT_LOGIN_HANDLER_H_
