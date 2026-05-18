/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gfxPlatformMinWayland.h"

#include "cairo.h"
#include "gfxFcPlatformFontList.h"
#include "gfxFT2FontBase.h"
#include "gfxImageSurface.h"
#include "gfxPlatformFontList.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "VsyncSource.h"

using namespace mozilla;
using namespace mozilla::gfx;

static FT_Library gPlatformFTLibrary = nullptr;

#define GFX_PREF_MAX_GENERIC_SUBSTITUTIONS \
  "gfx.font_rendering.fontconfig.max_generic_substitutions"

gfxPlatformMinWayland::gfxPlatformMinWayland() {
  InitBackendPrefs(GetBackendPrefs());

  gPlatformFTLibrary = Factory::NewFTLibrary();
  MOZ_RELEASE_ASSERT(gPlatformFTLibrary);
  Factory::SetFTLibrary(gPlatformFTLibrary);
}

gfxPlatformMinWayland::~gfxPlatformMinWayland() {
  Factory::ReleaseFTLibrary(gPlatformFTLibrary);
  gPlatformFTLibrary = nullptr;
}

void gfxPlatformMinWayland::InitAcceleration() {
  gfxPlatform::InitAcceleration();
  if (XRE_IsContentProcess()) {
    ImportCachedContentDeviceData();
  }
}

already_AddRefed<gfxASurface> gfxPlatformMinWayland::CreateOffscreenSurface(
    const IntSize& aSize, gfxImageFormat aFormat) {
  if (!Factory::AllowedSurfaceSize(aSize)) {
    return nullptr;
  }

  RefPtr<gfxASurface> surface = new gfxImageSurface(aSize, aFormat);
  if (surface->CairoStatus()) {
    return nullptr;
  }
  return surface.forget();
}

nsresult gfxPlatformMinWayland::GetFontList(
    nsAtom* aLangGroup, const nsACString& aGenericFamily,
    nsTArray<nsString>& aListOfFonts) {
  gfxPlatformFontList::PlatformFontList()->GetFontList(
      aLangGroup, aGenericFamily, aListOfFonts);
  return NS_OK;
}

static const char kFontDejaVuSans[] = "DejaVu Sans";
static const char kFontDejaVuSerif[] = "DejaVu Serif";
static const char kFontFreeSans[] = "FreeSans";
static const char kFontFreeSerif[] = "FreeSerif";
static const char kFontTwemojiMozilla[] = "Twemoji Mozilla";
static const char kFontSymbola[] = "Symbola";
static const char kFontNotoSansSymbols[] = "Noto Sans Symbols";
static const char kFontNotoSansSymbols2[] = "Noto Sans Symbols2";
static const char kFontDroidSansFallback[] = "Droid Sans Fallback";
static const char kFontWenQuanYiMicroHei[] = "WenQuanYi Micro Hei";
static const char kFontNanumGothic[] = "NanumGothic";

void gfxPlatformMinWayland::GetCommonFallbackFonts(
    uint32_t aCh, Script aRunScript, FontPresentation aPresentation,
    nsTArray<const char*>& aFontList) {
  if (PrefersColor(aPresentation)) {
    aFontList.AppendElement(kFontTwemojiMozilla);
  }

  aFontList.AppendElement(kFontDejaVuSerif);
  aFontList.AppendElement(kFontFreeSerif);
  aFontList.AppendElement(kFontDejaVuSans);
  aFontList.AppendElement(kFontFreeSans);
  aFontList.AppendElement(kFontSymbola);
  aFontList.AppendElement(kFontNotoSansSymbols);
  aFontList.AppendElement(kFontNotoSansSymbols2);

  if (aCh >= 0x3000 && ((aCh < 0xe000) || (aCh >= 0xf900 && aCh < 0xfff0) ||
                        ((aCh >> 16) == 2))) {
    aFontList.AppendElement(kFontDroidSansFallback);
    aFontList.AppendElement(kFontWenQuanYiMicroHei);
    aFontList.AppendElement(kFontNanumGothic);
  }
}

void gfxPlatformMinWayland::ReadSystemFontList(
    mozilla::dom::SystemFontList* aRetValue) {
  gfxFcPlatformFontList::PlatformFontList()->ReadSystemFontList(aRetValue);
}

bool gfxPlatformMinWayland::CreatePlatformFontList() {
  return gfxPlatformFontList::Initialize(new gfxFcPlatformFontList);
}

void gfxPlatformMinWayland::FontsPrefsChanged(const char* aPref) {
  if (strcmp(GFX_PREF_MAX_GENERIC_SUBSTITUTIONS, aPref) != 0) {
    gfxPlatform::FontsPrefsChanged(aPref);
    return;
  }

  gfxFcPlatformFontList::PlatformFontList()->ClearGenericMappings();
  FlushFontAndWordCaches();
}

gfxImageFormat gfxPlatformMinWayland::GetOffscreenFormat() {
  return SurfaceFormat::X8R8G8B8_UINT32;
}

already_AddRefed<mozilla::gfx::VsyncSource>
gfxPlatformMinWayland::CreateGlobalHardwareVsyncSource() {
  return GetSoftwareVsyncSource();
}

bool gfxPlatformMinWayland::CheckVariationFontSupport() {
  FT_Int major, minor, patch;
  FT_Library_Version(Factory::GetFTLibrary(), &major, &minor, &patch);
  return major * 1000000 + minor * 1000 + patch >= 2007001;
}
