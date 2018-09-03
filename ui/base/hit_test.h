// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_HIT_TEST_H_
#define UI_BASE_HIT_TEST_H_

#include "build/build_config.h"

// Defines the same symbolic names used by the WM_NCHITTEST Notification under
// win32 (the integer values are not guaranteed to be equivalent). We do this
// because we have a whole bunch of code that deals with window resizing and
// such that requires these values.

#if !defined(OS_WIN)

enum HitTestCompat
{
  HTNOWHERE = 0,
  HTBORDER,
  HTBOTTOM,
  HTBOTTOMLEFT,
  HTBOTTOMRIGHT,
  HTCAPTION,
  HTCLIENT,
  HTCLOSE,
  HTPIN,
  HTUNPIN,
  HTACCOUNTBUTTON,
  HTRETURN,
  HTERROR,
  HTGROWBOX,
  HTHELP,
  HTHSCROLL,
  HTLEFT,
  HTMENU,
  HTMAXBUTTON,
  HTMINBUTTON,
  HTTHEMEBUTTON,
  HTREDUCE,
  HTRIGHT,
  HTSIZE,
  HTSYSMENU,
  HTTOP,
  HTTOPLEFT,
  HTTOPRIGHT,
  HTTRANSPARENT,
  HTVSCROLL,
  HTZOOM
};

#elif defined(OS_WIN)

enum HitTestCompat
{
  HTPIN = 0,
  HTUNPIN,
  HTRETURN,
  HTTHEMEBUTTON,
  HTACCOUNTBUTTON
};

#endif

#endif  // UI_BASE_HIT_TEST_H_
