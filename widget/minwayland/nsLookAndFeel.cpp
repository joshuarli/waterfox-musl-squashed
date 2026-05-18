/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsLookAndFeel.h"

#include "mozilla/FontPropertyTypes.h"
#include "nsIContent.h"

using mozilla::ColorScheme;
using mozilla::FontSlantStyle;
using mozilla::FontStretch;
using mozilla::FontWeight;
using mozilla::StyleTextDecorationStyle;

nsLookAndFeel::nsLookAndFeel() = default;

nsLookAndFeel::~nsLookAndFeel() = default;

nsresult nsLookAndFeel::NativeGetColor(ColorID aID, ColorScheme aScheme,
                                       nscolor& aResult) {
  aResult = GetStandinForNativeColor(aID, aScheme);
  return NS_OK;
}

nsresult nsLookAndFeel::NativeGetInt(IntID aID, int32_t& aResult) {
  nsresult res = NS_OK;
  switch (aID) {
    case IntID::CaretBlinkTime:
      aResult = 567;
      break;
    case IntID::CaretWidth:
      aResult = 1;
      break;
    case IntID::SelectTextfieldsOnKeyFocus:
    case IntID::SkipNavigatingDisabledMenuItem:
    case IntID::ChosenMenuItemsShouldBlink:
      aResult = 1;
      break;
    case IntID::SubmenuDelay:
      aResult = 200;
      break;
    case IntID::DragThresholdX:
    case IntID::DragThresholdY:
      aResult = 4;
      break;
    case IntID::TreeOpenDelay:
    case IntID::TreeCloseDelay:
      aResult = 1000;
      break;
    case IntID::TreeLazyScrollDelay:
      aResult = 150;
      break;
    case IntID::TreeScrollDelay:
      aResult = 100;
      break;
    case IntID::TreeScrollLinesMax:
      aResult = 3;
      break;
    case IntID::AlertNotificationOrigin:
      aResult = NS_ALERT_TOP;
      break;
    case IntID::IMERawInputUnderlineStyle:
    case IntID::IMESelectedRawTextUnderlineStyle:
    case IntID::IMEConvertedTextUnderlineStyle:
    case IntID::IMESelectedConvertedTextUnderline:
      aResult = static_cast<int32_t>(StyleTextDecorationStyle::Solid);
      break;
    case IntID::SpellCheckerUnderlineStyle:
      aResult = static_cast<int32_t>(StyleTextDecorationStyle::Dotted);
      break;
    case IntID::ContextMenuOffsetVertical:
      aResult = -6;
      break;
    case IntID::ContextMenuOffsetHorizontal:
      aResult = 1;
      break;
    case IntID::ScrollButtonMiddleMouseButtonAction:
    case IntID::ScrollButtonRightMouseButtonAction:
      aResult = 3;
      break;
    case IntID::MenusCanOverlapOSBar:
    case IntID::UseOverlayScrollbars:
    case IntID::AllowOverlayScrollbarsOverlap:
    case IntID::UseAccessibilityTheme:
    case IntID::ScrollArrowStyle:
    case IntID::ScrollButtonLeftMouseButtonAction:
    case IntID::WindowsAccentColorInTitlebar:
    case IntID::ScrollToClick:
    case IntID::MenuBarDrag:
    case IntID::ScrollbarButtonAutoRepeatBehavior:
    case IntID::SwipeAnimationEnabled:
    case IntID::ScrollbarDisplayOnMouseMove:
    case IntID::ScrollbarFadeBeginDelay:
    case IntID::ScrollbarFadeDuration:
    case IntID::GTKCSDAvailable:
    case IntID::GTKCSDMinimizeButton:
    case IntID::GTKCSDMaximizeButton:
    case IntID::GTKCSDReversedPlacement:
    case IntID::SystemUsesDarkTheme:
    case IntID::PrefersReducedMotion:
    case IntID::PrefersReducedTransparency:
    case IntID::InvertedColors:
    case IntID::PrimaryPointerCapabilities:
    case IntID::AllPointerCapabilities:
      aResult = 0;
      break;
    case IntID::GTKCSDCloseButton:
      aResult = 1;
      break;
    default:
      aResult = 0;
      res = NS_ERROR_FAILURE;
      break;
  }
  return res;
}

nsresult nsLookAndFeel::NativeGetFloat(FloatID aID, float& aResult) {
  switch (aID) {
    case FloatID::IMEUnderlineRelativeSize:
    case FloatID::SpellCheckerUnderlineRelativeSize:
      aResult = 1.0f;
      return NS_OK;
    case FloatID::CaretAspectRatio:
    default:
      aResult = -1.0f;
      return NS_ERROR_FAILURE;
  }
}

bool nsLookAndFeel::NativeGetFont(FontID aID, nsString& aFontName,
                                  gfxFontStyle& aFontStyle) {
  aFontStyle.style = FontSlantStyle::NORMAL;
  aFontStyle.weight = FontWeight::NORMAL;
  aFontStyle.stretch = FontStretch::NORMAL;
  aFontStyle.size = 14;
  aFontStyle.systemFont = true;

  aFontName.AssignLiteral("sans-serif");
  return true;
}

char16_t nsLookAndFeel::GetPasswordCharacterImpl() { return 0x2022; }
