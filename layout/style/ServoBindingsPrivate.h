/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_ServoBindingsPrivate_h
#define mozilla_ServoBindingsPrivate_h

#include <sstream>
#include "mozilla/HashFunctions.h"

#ifdef RUST_BINDGEN
#  pragma push_macro("private")
#  pragma push_macro("protected")
#  undef private
#  undef protected
#  define private public
#  define protected public
#endif

#include "mozilla/dom/Element.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/dom/HTMLSlotElement.h"
#include "mozilla/AnimatedPropertyID.h"
#include "mozilla/ComputedStyle.h"
#include "mozilla/GeckoBindings.h"
#include "mozilla/ServoComputedData.h"
#include "mozilla/ServoElementSnapshot.h"
#include "mozilla/StyleSheet.h"
#include "mozilla/StyleSheetInfo.h"
#include "nsPresContext.h"
#include "AttrArray.h"
#include "nsIContent.h"
#include "nsINode.h"
#include "nsAttrName.h"
#include "nsAtom.h"
#include "mozilla/dom/FragmentOrElement.h"

#ifdef RUST_BINDGEN
#  pragma pop_macro("protected")
#  pragma pop_macro("private")
#endif

#endif
