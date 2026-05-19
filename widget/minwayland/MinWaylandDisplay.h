/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_minwayland_MinWaylandDisplay_h
#define widget_minwayland_MinWaylandDisplay_h

#include "Units.h"
#include "mozilla/EventForwards.h"
#include "mozilla/Mutex.h"
#include "nsTArray.h"

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct xdg_wm_base;

namespace mozilla::widget {

class MinWaylandWindow;

class MinWaylandDisplay final {
 public:
  static MinWaylandDisplay* Get();

  bool IsReady() const { return mDisplay && mCompositor && mShm && mXdgWmBase; }
  wl_display* Display() const { return mDisplay; }
  wl_compositor* Compositor() const { return mCompositor; }
  wl_shm* Shm() const { return mShm; }
  xdg_wm_base* XdgWmBase() const { return mXdgWmBase; }
  wl_data_device_manager* DataDeviceManager() const {
    return mDataDeviceManager;
  }
  wl_seat* Seat() const { return mSeat; }
  uint32_t LastInputSerial() const { return mLastInputSerial; }

  Mutex& WaylandMutex() { return mMutex; }

  void DispatchPending(bool aMayWait = false);
  bool Roundtrip();
  void Flush();
  void RegisterWindow(MinWaylandWindow* aWindow);
  void UnregisterWindow(MinWaylandWindow* aWindow);

  static void RegistryGlobal(void* aData, wl_registry* aRegistry,
                             uint32_t aName, const char* aInterface,
                             uint32_t aVersion);
  static void RegistryGlobalRemove(void* aData, wl_registry* aRegistry,
                                   uint32_t aName);
  static void XdgPing(void* aData, xdg_wm_base* aShell, uint32_t aSerial);
  static void SeatCapabilities(void* aData, wl_seat* aSeat,
                               uint32_t aCapabilities);
  static void SeatName(void* aData, wl_seat* aSeat, const char* aName);
  static void PointerEnter(void* aData, wl_pointer* aPointer, uint32_t aSerial,
                           wl_surface* aSurface, wl_fixed_t aSurfaceX,
                           wl_fixed_t aSurfaceY);
  static void PointerLeave(void* aData, wl_pointer* aPointer, uint32_t aSerial,
                           wl_surface* aSurface);
  static void PointerMotion(void* aData, wl_pointer* aPointer, uint32_t aTime,
                            wl_fixed_t aSurfaceX, wl_fixed_t aSurfaceY);
  static void PointerButton(void* aData, wl_pointer* aPointer,
                            uint32_t aSerial, uint32_t aTime, uint32_t aButton,
                            uint32_t aState);
  static void PointerAxis(void* aData, wl_pointer* aPointer, uint32_t aTime,
                          uint32_t aAxis, wl_fixed_t aValue);
  static void KeyboardKeymap(void* aData, wl_keyboard* aKeyboard,
                             uint32_t aFormat, int32_t aFd, uint32_t aSize);
  static void KeyboardEnter(void* aData, wl_keyboard* aKeyboard,
                            uint32_t aSerial, wl_surface* aSurface,
                            wl_array* aKeys);
  static void KeyboardLeave(void* aData, wl_keyboard* aKeyboard,
                            uint32_t aSerial, wl_surface* aSurface);
  static void KeyboardKey(void* aData, wl_keyboard* aKeyboard,
                          uint32_t aSerial, uint32_t aTime, uint32_t aKey,
                          uint32_t aState);
  static void KeyboardModifiers(void* aData, wl_keyboard* aKeyboard,
                                uint32_t aSerial, uint32_t aModsDepressed,
                                uint32_t aModsLatched, uint32_t aModsLocked,
                                uint32_t aGroup);
  static void KeyboardRepeatInfo(void* aData, wl_keyboard* aKeyboard,
                                 int32_t aRate, int32_t aDelay);
  ~MinWaylandDisplay();

 private:
  MinWaylandDisplay();
  MinWaylandWindow* WindowForSurface(wl_surface* aSurface) const;
  uint32_t CurrentModifiers() const;
  void DestroyKeyboardState();

  Mutex mMutex MOZ_UNANNOTATED{"MinWaylandDisplay"};
  wl_display* mDisplay = nullptr;
  wl_registry* mRegistry = nullptr;
  wl_compositor* mCompositor = nullptr;
  wl_shm* mShm = nullptr;
  xdg_wm_base* mXdgWmBase = nullptr;
  wl_data_device_manager* mDataDeviceManager = nullptr;
  wl_seat* mSeat = nullptr;
  wl_pointer* mPointer = nullptr;
  wl_keyboard* mKeyboard = nullptr;
  xkb_context* mXkbContext = nullptr;
  xkb_keymap* mXkbKeymap = nullptr;
  xkb_state* mXkbState = nullptr;
  nsTArray<MinWaylandWindow*> mWindows;
  MinWaylandWindow* mPointerWindow = nullptr;
  MinWaylandWindow* mKeyboardWindow = nullptr;
  LayoutDeviceIntPoint mPointerPoint;
  uint16_t mPointerButtons = 0;
  uint32_t mModifiers = 0;
  uint32_t mLastInputSerial = 0;
};

}  // namespace mozilla::widget

#endif
