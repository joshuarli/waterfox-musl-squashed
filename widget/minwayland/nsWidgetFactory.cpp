/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsWidgetFactory.h"

#include "MinWaylandClipboard.h"
#include "mozilla/Components.h"
#include "mozilla/WidgetUtils.h"
#include "nsAppShell.h"
#include "nsAppShellSingleton.h"
#include "nsBaseWidget.h"
#include "nsIClipboard.h"
#include "nsLookAndFeel.h"

NS_IMPL_COMPONENT_FACTORY(nsIClipboard) {
  nsCOMPtr<nsIClipboard> inst = new mozilla::widget::MinWaylandClipboard();
  return inst.forget().downcast<nsISupports>();
}

nsresult nsWidgetMinWaylandModuleCtor() { return nsAppShellInit(); }

void nsWidgetMinWaylandModuleDtor() {
  mozilla::widget::WidgetUtils::Shutdown();
  nsLookAndFeel::Shutdown();
  nsAppShellShutdown();
}
