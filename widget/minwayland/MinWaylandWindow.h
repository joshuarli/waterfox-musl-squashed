/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_minwayland_MinWaylandWindow_h
#define widget_minwayland_MinWaylandWindow_h

#include "HeadlessWidget.h"
#include "mozilla/EventForwards.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/2D.h"

#include <wayland-client.h>

struct xdg_surface;
struct xdg_toplevel;
struct xdg_popup;

namespace mozilla::widget {

class MinWaylandDisplay;

class MinWaylandWindow final : public HeadlessWidget {
 public:
  MinWaylandWindow();

  NS_INLINE_DECL_REFCOUNTING_INHERITED(MinWaylandWindow, HeadlessWidget)

  nsresult Create(nsIWidget* aParent, const LayoutDeviceIntRect& aRect,
                  widget::InitData* aInitData = nullptr) override;
  using HeadlessWidget::Create;

  void Destroy() override;
  void Show(bool aState) override;
  void Resize(double aWidth, double aHeight, bool aRepaint) override;
  void Resize(double aX, double aY, double aWidth, double aHeight,
              bool aRepaint) override;
  nsresult SetTitle(const nsAString& aTitle) override;
  void* GetNativeData(uint32_t aDataType) override;
  wl_surface* Surface() const { return mSurface; }
  void DispatchPointerEnter(const LayoutDeviceIntPoint& aPoint,
                            uint32_t aModifiers);
  void DispatchPointerLeave(const LayoutDeviceIntPoint& aPoint,
                            uint32_t aModifiers);
  void DispatchPointerMotion(const LayoutDeviceIntPoint& aPoint,
                             uint32_t aModifiers);
  void DispatchPointerButton(EventMessage aMessage, MouseButton aButton,
                             const LayoutDeviceIntPoint& aPoint,
                             uint32_t aModifiers, uint16_t aButtons);
  void DispatchPointerAxis(double aDeltaX, double aDeltaY,
                           const LayoutDeviceIntPoint& aPoint,
                           uint32_t aModifiers);
  void DispatchKeyboard(uint32_t aXkbKeycode, uint32_t aKeysym,
                        const nsACString& aUtf8, bool aPressed,
                        uint32_t aModifiers);

  struct ShmBuffer;

  static void XdgSurfaceConfigure(void* aData, xdg_surface* aSurface,
                                  uint32_t aSerial);
  static void XdgToplevelConfigure(void* aData, xdg_toplevel* aToplevel,
                                   int32_t aWidth, int32_t aHeight,
                                   wl_array* aStates);
  static void XdgToplevelClose(void* aData, xdg_toplevel* aToplevel);
  static void XdgPopupConfigure(void* aData, xdg_popup* aPopup, int32_t aX,
                                int32_t aY, int32_t aWidth, int32_t aHeight);
  static void XdgPopupDone(void* aData, xdg_popup* aPopup);
  static void XdgPopupRepositioned(void* aData, xdg_popup* aPopup,
                                   uint32_t aToken);

 protected:
  ~MinWaylandWindow() override;

  already_AddRefed<mozilla::gfx::DrawTarget> StartRemoteDrawingInRegion(
      const LayoutDeviceIntRegion& aInvalidRegion) override;
  void EndRemoteDrawingInRegion(
      mozilla::gfx::DrawTarget* aDrawTarget,
      const LayoutDeviceIntRegion& aInvalidRegion) override;
  void CleanupRemoteDrawing() override;

 private:
  bool EnsureNativeWindow();
  bool EnsureToplevelWindow();
  bool EnsurePopupWindow();
  void MaybeShowPopup();
  UniquePtr<ShmBuffer> CreateBuffer(const LayoutDeviceIntSize& aSize);
  ShmBuffer* LatestBuffer() const;
  void PresentBuffer(UniquePtr<ShmBuffer> aBuffer,
                     const LayoutDeviceIntRegion& aInvalidRegion);
  void DestroyNativeWindow();
  void PruneReleasedBuffers();
  LayoutDeviceIntSize BufferSize();

  MinWaylandDisplay* mDisplay = nullptr;
  MinWaylandWindow* mParentWindow = nullptr;
  wl_surface* mSurface = nullptr;
  xdg_surface* mXdgSurface = nullptr;
  xdg_toplevel* mToplevel = nullptr;
  xdg_popup* mPopup = nullptr;
  bool mConfigured = false;
  uint8_t mPopupShowRetries = 0;
  nsString mTitle;
  LayoutDeviceIntSize mConfiguredSize;
  UniquePtr<ShmBuffer> mDrawingBuffer;
  nsTArray<UniquePtr<ShmBuffer>> mLiveBuffers;
};

}  // namespace mozilla::widget

#endif
