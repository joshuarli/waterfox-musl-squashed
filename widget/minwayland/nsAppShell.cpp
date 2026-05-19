/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAppShell.h"

#include "HeadlessScreenHelper.h"
#include "MinWaylandDisplay.h"
#include "mozilla/Hal.h"
#include "mozilla/widget/ScreenManager.h"

using mozilla::MakeUnique;
using mozilla::widget::MinWaylandDisplay;
using mozilla::widget::ScreenManager;

nsresult nsAppShell::Init() {
  mozilla::hal::Init();
  if (XRE_IsParentProcess()) {
    ScreenManager::GetSingleton().SetHelper(
        MakeUnique<mozilla::widget::HeadlessScreenHelper>());
  }
  return nsBaseAppShell::Init();
}

nsAppShell::~nsAppShell() { mozilla::hal::Shutdown(); }

void nsAppShell::ScheduleNativeEventCallback() {}

bool nsAppShell::ProcessNextNativeEvent(bool aMayWait) {
  if (MinWaylandDisplay* display = MinWaylandDisplay::Get()) {
    display->DispatchPending(aMayWait);
  }
  return false;
}
