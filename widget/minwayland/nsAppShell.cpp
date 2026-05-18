/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAppShell.h"

#include "mozilla/RefPtr.h"
#include "nsThreadUtils.h"

nsresult nsAppShell::Init() { return nsBaseAppShell::Init(); }

void nsAppShell::ScheduleNativeEventCallback() {
  NS_DispatchToMainThread(
      NS_NewRunnableFunction("minwayland native event callback",
                             [self = RefPtr<nsAppShell>(this)] {
                               self->NativeEventCallback();
                             }));
}

bool nsAppShell::ProcessNextNativeEvent(bool aMayWait) { return false; }
