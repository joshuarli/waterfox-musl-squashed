/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MinWaylandDisplay.h"

#include "MinWaylandWindow.h"
#include "mozilla/Logging.h"
#include "mozilla/ScopeExit.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/widget/xdg-shell-client-protocol.h"
#include "nsString.h"

#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

namespace mozilla::widget {

static StaticAutoPtr<MinWaylandDisplay> sDisplay;
static LazyLogModule sMinWaylandDisplayLog("MinWayland");

#define LOG(...) MOZ_LOG(sMinWaylandDisplayLog, LogLevel::Debug, (__VA_ARGS__))

static const wl_registry_listener kRegistryListener = {
    MinWaylandDisplay::RegistryGlobal,
    MinWaylandDisplay::RegistryGlobalRemove,
};

static const xdg_wm_base_listener kXdgWmBaseListener = {
    MinWaylandDisplay::XdgPing,
};

static const wl_seat_listener kSeatListener = {
    MinWaylandDisplay::SeatCapabilities,
    MinWaylandDisplay::SeatName,
};

static const wl_pointer_listener kPointerListener = {
    MinWaylandDisplay::PointerEnter,  MinWaylandDisplay::PointerLeave,
    MinWaylandDisplay::PointerMotion, MinWaylandDisplay::PointerButton,
    MinWaylandDisplay::PointerAxis,
};

static const wl_keyboard_listener kKeyboardListener = {
    MinWaylandDisplay::KeyboardKeymap,    MinWaylandDisplay::KeyboardEnter,
    MinWaylandDisplay::KeyboardLeave,     MinWaylandDisplay::KeyboardKey,
    MinWaylandDisplay::KeyboardModifiers, MinWaylandDisplay::KeyboardRepeatInfo,
};

MinWaylandDisplay* MinWaylandDisplay::Get() {
  if (!sDisplay) {
    sDisplay = new MinWaylandDisplay();
  }
  return sDisplay;
}

MinWaylandDisplay::MinWaylandDisplay() {
  mXkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  mDisplay = wl_display_connect(nullptr);
  if (!mDisplay) {
    return;
  }

  mRegistry = wl_display_get_registry(mDisplay);
  wl_registry_add_listener(mRegistry, &kRegistryListener, this);
  Roundtrip();
  Roundtrip();
}

MinWaylandDisplay::~MinWaylandDisplay() {
  DestroyKeyboardState();
  if (mKeyboard) {
    wl_keyboard_destroy(mKeyboard);
  }
  if (mPointer) {
    wl_pointer_destroy(mPointer);
  }
  if (mSeat) {
    wl_seat_destroy(mSeat);
  }
  if (mXdgWmBase) {
    xdg_wm_base_destroy(mXdgWmBase);
  }
  if (mDataDeviceManager) {
    wl_data_device_manager_destroy(mDataDeviceManager);
  }
  if (mShm) {
    wl_shm_destroy(mShm);
  }
  if (mCompositor) {
    wl_compositor_destroy(mCompositor);
  }
  if (mRegistry) {
    wl_registry_destroy(mRegistry);
  }
  if (mDisplay) {
    wl_display_disconnect(mDisplay);
  }
  if (mXkbContext) {
    xkb_context_unref(mXkbContext);
  }
}

void MinWaylandDisplay::RegistryGlobal(void* aData, wl_registry* aRegistry,
                                       uint32_t aName, const char* aInterface,
                                       uint32_t aVersion) {
  auto* display = static_cast<MinWaylandDisplay*>(aData);
  if (!strcmp(aInterface, wl_compositor_interface.name)) {
    display->mCompositor = static_cast<wl_compositor*>(wl_registry_bind(
        aRegistry, aName, &wl_compositor_interface, std::min(aVersion, 4u)));
  } else if (!strcmp(aInterface, wl_shm_interface.name)) {
    display->mShm = static_cast<wl_shm*>(
        wl_registry_bind(aRegistry, aName, &wl_shm_interface, 1));
  } else if (!strcmp(aInterface, xdg_wm_base_interface.name)) {
    display->mXdgWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(
        aRegistry, aName, &xdg_wm_base_interface, std::min(aVersion, 1u)));
    xdg_wm_base_add_listener(display->mXdgWmBase, &kXdgWmBaseListener,
                             display);
  } else if (!strcmp(aInterface, wl_data_device_manager_interface.name)) {
    display->mDataDeviceManager =
        static_cast<wl_data_device_manager*>(wl_registry_bind(
            aRegistry, aName, &wl_data_device_manager_interface,
            std::min(aVersion, 3u)));
  } else if (!strcmp(aInterface, wl_seat_interface.name)) {
    display->mSeat = static_cast<wl_seat*>(wl_registry_bind(
        aRegistry, aName, &wl_seat_interface, std::min(aVersion, 4u)));
    wl_seat_add_listener(display->mSeat, &kSeatListener, display);
  }
}

void MinWaylandDisplay::RegistryGlobalRemove(void* aData,
                                             wl_registry* aRegistry,
                                             uint32_t aName) {}

void MinWaylandDisplay::XdgPing(void* aData, xdg_wm_base* aShell,
                                uint32_t aSerial) {
  xdg_wm_base_pong(aShell, aSerial);
}

void MinWaylandDisplay::DispatchPending(bool aMayWait) {
  if (!mDisplay) {
    return;
  }
  while (wl_display_dispatch_pending(mDisplay) > 0) {
  }

  bool prepared = false;
  while (!prepared) {
    if (wl_display_prepare_read(mDisplay) == 0) {
      prepared = true;
      break;
    }
    if (wl_display_dispatch_pending(mDisplay) <= 0) {
      break;
    }
  }

  if (prepared) {
    bool shouldRead = false;
    if (wl_display_flush(mDisplay) >= 0 || errno == EAGAIN) {
      pollfd pfd = {wl_display_get_fd(mDisplay), POLLIN, 0};
      shouldRead = poll(&pfd, 1, aMayWait ? 16 : 0) > 0 &&
                   (pfd.revents & POLLIN);
    }
    if (shouldRead) {
      wl_display_read_events(mDisplay);
    } else {
      wl_display_cancel_read(mDisplay);
    }
  }

  while (wl_display_dispatch_pending(mDisplay) > 0) {
  }
  wl_display_flush(mDisplay);
}

bool MinWaylandDisplay::Roundtrip() {
  if (!mDisplay) {
    return false;
  }
  MutexAutoLock lock(mMutex);
  return wl_display_roundtrip(mDisplay) >= 0;
}

void MinWaylandDisplay::Flush() {
  if (!mDisplay) {
    return;
  }
  MutexAutoLock lock(mMutex);
  wl_display_flush(mDisplay);
}

void MinWaylandDisplay::RegisterWindow(MinWaylandWindow* aWindow) {
  if (!aWindow || mWindows.Contains(aWindow)) {
    return;
  }
  mWindows.AppendElement(aWindow);
}

void MinWaylandDisplay::UnregisterWindow(MinWaylandWindow* aWindow) {
  mWindows.RemoveElement(aWindow);
  if (mPointerWindow == aWindow) {
    mPointerWindow = nullptr;
  }
  if (mKeyboardWindow == aWindow) {
    mKeyboardWindow = nullptr;
  }
}

MinWaylandWindow* MinWaylandDisplay::WindowForSurface(
    wl_surface* aSurface) const {
  for (MinWaylandWindow* window : mWindows) {
    if (window && window->Surface() == aSurface) {
      return window;
    }
  }
  return nullptr;
}

void MinWaylandDisplay::SeatCapabilities(void* aData, wl_seat* aSeat,
                                         uint32_t aCapabilities) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if ((aCapabilities & WL_SEAT_CAPABILITY_POINTER) && !self->mPointer) {
    self->mPointer = wl_seat_get_pointer(aSeat);
    wl_pointer_add_listener(self->mPointer, &kPointerListener, self);
  } else if (!(aCapabilities & WL_SEAT_CAPABILITY_POINTER) && self->mPointer) {
    wl_pointer_destroy(self->mPointer);
    self->mPointer = nullptr;
    self->mPointerWindow = nullptr;
  }

  if ((aCapabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !self->mKeyboard) {
    self->mKeyboard = wl_seat_get_keyboard(aSeat);
    wl_keyboard_add_listener(self->mKeyboard, &kKeyboardListener, self);
  } else if (!(aCapabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
             self->mKeyboard) {
    wl_keyboard_destroy(self->mKeyboard);
    self->mKeyboard = nullptr;
    self->mKeyboardWindow = nullptr;
    self->DestroyKeyboardState();
  }
}

void MinWaylandDisplay::SeatName(void* aData, wl_seat* aSeat,
                                 const char* aName) {}

void MinWaylandDisplay::PointerEnter(void* aData, wl_pointer* aPointer,
                                     uint32_t aSerial, wl_surface* aSurface,
                                     wl_fixed_t aSurfaceX,
                                     wl_fixed_t aSurfaceY) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  self->mPointerWindow = self->WindowForSurface(aSurface);
  self->mPointerPoint = LayoutDeviceIntPoint(wl_fixed_to_int(aSurfaceX),
                                             wl_fixed_to_int(aSurfaceY));
  if (self->mPointerWindow) {
    self->mPointerWindow->DispatchPointerEnter(self->mPointerPoint,
                                               self->CurrentModifiers());
  }
}

void MinWaylandDisplay::PointerLeave(void* aData, wl_pointer* aPointer,
                                     uint32_t aSerial, wl_surface* aSurface) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if (self->mPointerButtons) {
    return;
  }
  if (self->mPointerWindow) {
    self->mPointerWindow->DispatchPointerLeave(self->mPointerPoint,
                                               self->CurrentModifiers());
  }
  self->mPointerWindow = nullptr;
}

void MinWaylandDisplay::PointerMotion(void* aData, wl_pointer* aPointer,
                                      uint32_t aTime, wl_fixed_t aSurfaceX,
                                      wl_fixed_t aSurfaceY) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  self->mPointerPoint = LayoutDeviceIntPoint(wl_fixed_to_int(aSurfaceX),
                                             wl_fixed_to_int(aSurfaceY));
  if (self->mPointerWindow) {
    self->mPointerWindow->DispatchPointerMotion(self->mPointerPoint,
                                                self->CurrentModifiers());
  }
}

void MinWaylandDisplay::PointerButton(void* aData, wl_pointer* aPointer,
                                      uint32_t aSerial, uint32_t aTime,
                                      uint32_t aButton, uint32_t aState) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if (!self->mPointerWindow) {
    return;
  }
  MouseButton button = MouseButton::ePrimary;
  if (aButton == BTN_RIGHT) {
    button = MouseButton::eSecondary;
  } else if (aButton == BTN_MIDDLE) {
    button = MouseButton::eMiddle;
  } else if (aButton != BTN_LEFT) {
    return;
  }

  EventMessage message = aState == WL_POINTER_BUTTON_STATE_PRESSED ? eMouseDown
                                                                   : eMouseUp;
  uint16_t buttonFlag = MouseButtonsFlagToChange(button);
  if (aState == WL_POINTER_BUTTON_STATE_PRESSED) {
    self->mLastInputSerial = aSerial;
    self->mPointerButtons |= buttonFlag;
  } else {
    self->mPointerButtons &= ~buttonFlag;
  }
  self->mPointerWindow->DispatchPointerButton(message, button,
                                              self->mPointerPoint,
                                              self->CurrentModifiers(),
                                              self->mPointerButtons);
}

void MinWaylandDisplay::PointerAxis(void* aData, wl_pointer* aPointer,
                                    uint32_t aTime, uint32_t aAxis,
                                    wl_fixed_t aValue) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if (!self->mPointerWindow) {
    return;
  }
  double deltaX = 0.0;
  double deltaY = 0.0;
  if (aAxis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
    deltaX = wl_fixed_to_double(aValue);
  } else if (aAxis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    deltaY = wl_fixed_to_double(aValue);
  }
  self->mPointerWindow->DispatchPointerAxis(deltaX, deltaY, self->mPointerPoint,
                                            self->CurrentModifiers());
}

void MinWaylandDisplay::KeyboardKeymap(void* aData, wl_keyboard* aKeyboard,
                                       uint32_t aFormat, int32_t aFd,
                                       uint32_t aSize) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  auto closeFd = MakeScopeExit([&] { close(aFd); });
  if (aFormat != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 || !self->mXkbContext) {
    return;
  }

  void* data = mmap(nullptr, aSize, PROT_READ, MAP_PRIVATE, aFd, 0);
  if (data == MAP_FAILED) {
    return;
  }
  auto unmapData = MakeScopeExit([&] { munmap(data, aSize); });
  xkb_keymap* keymap = xkb_keymap_new_from_string(
      self->mXkbContext, static_cast<const char*>(data),
      XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    return;
  }
  xkb_state* state = xkb_state_new(keymap);
  if (!state) {
    xkb_keymap_unref(keymap);
    return;
  }

  self->DestroyKeyboardState();
  self->mXkbKeymap = keymap;
  self->mXkbState = state;
}

void MinWaylandDisplay::KeyboardEnter(void* aData, wl_keyboard* aKeyboard,
                                      uint32_t aSerial, wl_surface* aSurface,
                                      wl_array* aKeys) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  self->mKeyboardWindow = self->WindowForSurface(aSurface);
  LOG("keyboard enter surface=%p window=%p", aSurface, self->mKeyboardWindow);
}

void MinWaylandDisplay::KeyboardLeave(void* aData, wl_keyboard* aKeyboard,
                                      uint32_t aSerial, wl_surface* aSurface) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  LOG("keyboard leave surface=%p window=%p", aSurface, self->mKeyboardWindow);
  if (self->mKeyboardWindow == self->WindowForSurface(aSurface)) {
    self->mKeyboardWindow = nullptr;
  }
}

void MinWaylandDisplay::KeyboardKey(void* aData, wl_keyboard* aKeyboard,
                                    uint32_t aSerial, uint32_t aTime,
                                    uint32_t aKey, uint32_t aState) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if (!self->mKeyboardWindow || !self->mXkbState) {
    return;
  }
  const uint32_t keycode = aKey + 8;
  const xkb_keysym_t sym = xkb_state_key_get_one_sym(self->mXkbState, keycode);
  char utf8[64] = {};
  int length = xkb_state_key_get_utf8(self->mXkbState, keycode, utf8,
                                      sizeof(utf8));
  nsDependentCSubstring text(utf8, length > 0 ? std::min(length, 63) : 0);
  LOG("keyboard key keycode=%u sym=0x%x state=%u text='%s' modifiers=0x%x",
      keycode, sym, aState, PromiseFlatCString(text).get(),
      self->CurrentModifiers());
  if (aState == WL_KEYBOARD_KEY_STATE_PRESSED) {
    self->mLastInputSerial = aSerial;
  }
  self->mKeyboardWindow->DispatchKeyboard(
      keycode, sym, text, aState == WL_KEYBOARD_KEY_STATE_PRESSED,
      self->CurrentModifiers());
}

void MinWaylandDisplay::KeyboardModifiers(
    void* aData, wl_keyboard* aKeyboard, uint32_t aSerial,
    uint32_t aModsDepressed, uint32_t aModsLatched, uint32_t aModsLocked,
    uint32_t aGroup) {
  auto* self = static_cast<MinWaylandDisplay*>(aData);
  if (!self->mXkbState) {
    return;
  }
  xkb_state_update_mask(self->mXkbState, aModsDepressed, aModsLatched,
                        aModsLocked, 0, 0, aGroup);
  self->mModifiers = self->CurrentModifiers();
}

void MinWaylandDisplay::KeyboardRepeatInfo(void* aData,
                                           wl_keyboard* aKeyboard,
                                           int32_t aRate, int32_t aDelay) {}

uint32_t MinWaylandDisplay::CurrentModifiers() const {
  if (!mXkbState) {
    return mModifiers;
  }

  uint32_t modifiers = 0;
  auto active = [&](const char* aName) {
    return xkb_state_mod_name_is_active(mXkbState, aName,
                                        XKB_STATE_MODS_EFFECTIVE) > 0;
  };
  if (active(XKB_MOD_NAME_SHIFT)) {
    modifiers |= MODIFIER_SHIFT;
  }
  if (active(XKB_MOD_NAME_CTRL)) {
    modifiers |= MODIFIER_CONTROL;
  }
  if (active(XKB_MOD_NAME_ALT)) {
    modifiers |= MODIFIER_ALT;
  }
  if (active(XKB_MOD_NAME_LOGO)) {
    modifiers |= MODIFIER_META | MODIFIER_CONTROL;
  }
  if (active(XKB_MOD_NAME_CAPS)) {
    modifiers |= MODIFIER_CAPSLOCK;
  }
  if (active(XKB_MOD_NAME_NUM)) {
    modifiers |= MODIFIER_NUMLOCK;
  }
  return modifiers;
}

void MinWaylandDisplay::DestroyKeyboardState() {
  if (mXkbState) {
    xkb_state_unref(mXkbState);
    mXkbState = nullptr;
  }
  if (mXkbKeymap) {
    xkb_keymap_unref(mXkbKeymap);
    mXkbKeymap = nullptr;
  }
  mModifiers = 0;
}

}  // namespace mozilla::widget
