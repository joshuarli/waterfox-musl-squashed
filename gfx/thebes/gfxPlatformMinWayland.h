/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_PLATFORM_MINWAYLAND_H
#define GFX_PLATFORM_MINWAYLAND_H

#include "gfxPlatform.h"

class gfxPlatformMinWayland final : public gfxPlatform {
 public:
  gfxPlatformMinWayland();
  ~gfxPlatformMinWayland() override;

  static gfxPlatformMinWayland* GetPlatform() {
    return static_cast<gfxPlatformMinWayland*>(gfxPlatform::GetPlatform());
  }

  already_AddRefed<gfxASurface> CreateOffscreenSurface(
      const IntSize& aSize, gfxImageFormat aFormat) override;

  nsresult GetFontList(nsAtom* aLangGroup, const nsACString& aGenericFamily,
                       nsTArray<nsString>& aListOfFonts) override;
  void GetCommonFallbackFonts(uint32_t aCh, Script aRunScript,
                              FontPresentation aPresentation,
                              nsTArray<const char*>& aFontList) override;
  void ReadSystemFontList(mozilla::dom::SystemFontList* aRetValue) override;
  bool CreatePlatformFontList() override;
  void FontsPrefsChanged(const char* aPref) override;

  gfxImageFormat GetOffscreenFormat() override;
  bool SupportsApzWheelInput() const override { return true; }
  bool IsWaylandDisplay() override { return !gfxPlatform::IsHeadless(); }
  bool AccelerateLayersByDefault() override { return false; }
  already_AddRefed<mozilla::gfx::VsyncSource> CreateGlobalHardwareVsyncSource()
      override;

  static bool CheckVariationFontSupport();

 protected:
  void InitAcceleration() override;
};

#endif
