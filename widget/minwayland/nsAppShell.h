/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_minwayland_nsAppShell_h
#define widget_minwayland_nsAppShell_h

#include "nsBaseAppShell.h"

class nsAppShell final : public nsBaseAppShell {
 public:
  nsresult Init();

 protected:
  ~nsAppShell() override;

  void ScheduleNativeEventCallback() override;
  bool ProcessNextNativeEvent(bool aMayWait) override;
};

#endif
