/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *  Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA
 */

#include "core/frame/Navigator.h"

#include "bindings/core/v8/ScriptController.h"
#include "core/dom/Document.h"
#include "core/frame/FrameHost.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/NavigatorID.h"
#include "core/frame/Settings.h"
#include "core/loader/CookieJar.h"
#include "core/loader/FrameLoader.h"
#include "core/page/ChromeClient.h"
#include "platform/Language.h"

namespace blink {

Navigator::Navigator(LocalFrame* frame) : DOMWindowProperty(frame) {}

String Navigator::productSub() const {
  return "20030107";
}

String Navigator::vendor() const {
  // Do not change without good cause. History:
  // https://code.google.com/p/chromium/issues/detail?id=276813
  // https://www.w3.org/Bugs/Public/show_bug.cgi?id=27786
  // https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/QrgyulnqvmE
  return "Google Inc.";
}

String Navigator::vendorSub() const {
  return "";
}

String Navigator::userAgent() const {
  // If the frame is already detached it no longer has a meaningful useragent.
  if (!frame() || !frame()->page())
    return String();

  String url = frame()->document()->url().getString();

  if (url.contains("https://i.bank.ecitic.com")) {
    return "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1590.0 Safari/537.36";
  }

  if (url.contains("https://pbsz.ebank.cmbchina.com/CmbBank_GenShell/UI/GenShellPC/Login") ||
      url.contains("enterprise.bank.ecitic.com") || url.contains("https://netbank.nccbank.com.cn") ||
      url.contains("https://ebanks.bankofshanghai.com") || url.contains("https://online.hsbank.cc") ||
      url.contains("https://www.bankoftianjin.com") || url.contains("https://pibs.zyebank.com") ||
      url.contains("https://pbank.klb.cn/opweb") || url.contains("https://ebank.cqcbank.com") ||
      url.contains("https://www.bankgy.cn") || url.contains("https://ebank.trcbank.com.cn")) {
    return "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)";
  }

  String original_ua = frame()->loader().userAgent();
  if (url.contains("https://124.16.136.129") && !original_ua.contains("Windows"))  {
    return "Mozilla/5.0 AppleWebKit/537.32 (KHTML, like Gecko) ";
  }

  if (url.contains("https://www.tenpay.com")) {
#if defined(OS_WIN)
   return "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:46.0) Gecko/20100101 Firefox/46.0";
#else 
   return "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 6.0)";
#endif
  }

  // 华夏银行、平安银行伪装成IE
  if (url.contains("https://bank.pingan.com.cn") || url.contains("hxb.com.cn"))  {
    return "Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0;)";
  }
  
  return original_ua;
}

String Navigator::url() const {
  if (!frame() || !frame()->page())
    return String();

  return frame()->document()->url().getString();
}


String Navigator::platform() {
  if (!frame() || !frame()->page())
    return NavigatorID::platform();

  String url = frame()->document()->url().getString();

  if (url.contains("www.tenpay.com")) {
    return "Win32";
  }

  return NavigatorID::platform();
}

bool Navigator::cookieEnabled() const {
  if (!frame())
    return false;

  Settings* settings = frame()->settings();
  if (!settings || !settings->cookieEnabled())
    return false;

  return cookiesEnabled(frame()->document());
}

Vector<String> Navigator::languages() {
  Vector<String> languages;

  if (!frame() || !frame()->host()) {
    languages.append(defaultLanguage());
    return languages;
  }

  String acceptLanguages = frame()->host()->chromeClient().acceptLanguages();
  acceptLanguages.split(',', languages);

  // Sanitizing tokens. We could do that more extensively but we should assume
  // that the accept languages are already sane and support BCP47. It is
  // likely a waste of time to make sure the tokens matches that spec here.
  for (size_t i = 0; i < languages.size(); ++i) {
    String& token = languages[i];
    token = token.stripWhiteSpace();
    if (token.length() >= 3 && token[2] == '_')
      token.replace(2, 1, "-");
  }

  return languages;
}

DEFINE_TRACE(Navigator) {
  DOMWindowProperty::trace(visitor);
  Supplementable<Navigator>::trace(visitor);
}

}  // namespace blink
