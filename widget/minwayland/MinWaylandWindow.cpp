/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MinWaylandWindow.h"

#include "MinWaylandDisplay.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Logging.h"
#include "mozilla/ScopeExit.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TextEventDispatcher.h"
#include "mozilla/dom/WheelEventBinding.h"
#include "mozilla/widget/xdg-shell-client-protocol.h"
#include "nsReadableUtils.h"
#include "nsThreadUtils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mozilla::widget {

static LazyLogModule sMinWaylandLog("MinWayland");

#define LOG(...) MOZ_LOG(sMinWaylandLog, LogLevel::Debug, (__VA_ARGS__))

struct MinWaylandWindow::ShmBuffer {
  int mFd = -1;
  void* mData = MAP_FAILED;
  size_t mLength = 0;
  int32_t mStride = 0;
  wl_buffer* mBuffer = nullptr;
  RefPtr<gfx::DrawTarget> mDrawTarget;
  bool mReleased = false;

  ~ShmBuffer() {
    if (mBuffer) {
      wl_buffer_destroy(mBuffer);
    }
    if (mData != MAP_FAILED) {
      munmap(mData, mLength);
    }
    if (mFd >= 0) {
      close(mFd);
    }
  }
};

static void BufferRelease(void* aData, wl_buffer* aBuffer) {
  static_cast<MinWaylandWindow::ShmBuffer*>(aData)->mReleased = true;
}

static const wl_buffer_listener kBufferListener = {
    BufferRelease,
};

static const xdg_surface_listener kXdgSurfaceListener = {
    MinWaylandWindow::XdgSurfaceConfigure,
};

static const xdg_toplevel_listener kXdgToplevelListener = {
    MinWaylandWindow::XdgToplevelConfigure,
    MinWaylandWindow::XdgToplevelClose,
};

static const xdg_popup_listener kXdgPopupListener = {
    MinWaylandWindow::XdgPopupConfigure,
    MinWaylandWindow::XdgPopupDone,
    MinWaylandWindow::XdgPopupRepositioned,
};

static int CreateMemfd(size_t aLength) {
  int fd = static_cast<int>(
      syscall(SYS_memfd_create, "minwayland-buffer", MFD_CLOEXEC));
  if (fd < 0) {
    return -1;
  }
  if (ftruncate(fd, aLength) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

struct MinWaylandKeyInfo {
  uint32_t mKeyCode = NS_VK_UNKNOWN;
  KeyNameIndex mKeyNameIndex = KEY_NAME_INDEX_Unidentified;
  char16_t mChar = 0;
};

static MinWaylandKeyInfo KeyInfoFromKeysym(uint32_t aKeysym,
                                           const nsAString& aKeyValue) {
  switch (aKeysym) {
    case XKB_KEY_BackSpace:
      return {NS_VK_BACK, KEY_NAME_INDEX_Backspace, 0};
    case XKB_KEY_Tab:
    case XKB_KEY_ISO_Left_Tab:
      return {NS_VK_TAB, KEY_NAME_INDEX_Tab, 0};
    case XKB_KEY_Return:
    case XKB_KEY_KP_Enter:
      return {NS_VK_RETURN, KEY_NAME_INDEX_Enter, 0};
    case XKB_KEY_Escape:
      return {NS_VK_ESCAPE, KEY_NAME_INDEX_Escape, 0};
    case XKB_KEY_Delete:
    case XKB_KEY_KP_Delete:
      return {NS_VK_DELETE, KEY_NAME_INDEX_Delete, 0};
    case XKB_KEY_Left:
    case XKB_KEY_KP_Left:
      return {NS_VK_LEFT, KEY_NAME_INDEX_ArrowLeft, 0};
    case XKB_KEY_Right:
    case XKB_KEY_KP_Right:
      return {NS_VK_RIGHT, KEY_NAME_INDEX_ArrowRight, 0};
    case XKB_KEY_Up:
    case XKB_KEY_KP_Up:
      return {NS_VK_UP, KEY_NAME_INDEX_ArrowUp, 0};
    case XKB_KEY_Down:
    case XKB_KEY_KP_Down:
      return {NS_VK_DOWN, KEY_NAME_INDEX_ArrowDown, 0};
    case XKB_KEY_Home:
    case XKB_KEY_KP_Home:
      return {NS_VK_HOME, KEY_NAME_INDEX_Home, 0};
    case XKB_KEY_End:
    case XKB_KEY_KP_End:
      return {NS_VK_END, KEY_NAME_INDEX_End, 0};
    case XKB_KEY_Page_Up:
    case XKB_KEY_KP_Page_Up:
      return {NS_VK_PAGE_UP, KEY_NAME_INDEX_PageUp, 0};
    case XKB_KEY_Page_Down:
    case XKB_KEY_KP_Page_Down:
      return {NS_VK_PAGE_DOWN, KEY_NAME_INDEX_PageDown, 0};
    case XKB_KEY_Shift_L:
    case XKB_KEY_Shift_R:
      return {NS_VK_SHIFT, KEY_NAME_INDEX_Shift, 0};
    case XKB_KEY_Control_L:
    case XKB_KEY_Control_R:
      return {NS_VK_CONTROL, KEY_NAME_INDEX_Control, 0};
    case XKB_KEY_Alt_L:
    case XKB_KEY_Alt_R:
      return {NS_VK_ALT, KEY_NAME_INDEX_Alt, 0};
    case XKB_KEY_Meta_L:
    case XKB_KEY_Meta_R:
    case XKB_KEY_Super_L:
    case XKB_KEY_Super_R:
      return {NS_VK_META, KEY_NAME_INDEX_Meta, 0};
    default:
      break;
  }

  char16_t ch = 0;
  if (aKeyValue.Length() == 1 && aKeyValue[0] >= ' ' &&
      aKeyValue[0] != 0x7f) {
    ch = aKeyValue[0];
  } else if (aKeysym >= XKB_KEY_space && aKeysym <= XKB_KEY_asciitilde) {
    ch = static_cast<char16_t>(aKeysym);
  }
  if (ch) {
    if (ch >= 'a' && ch <= 'z') {
      return {uint32_t(NS_VK_A + (ch - 'a')), KEY_NAME_INDEX_USE_STRING, ch};
    }
    if (ch >= 'A' && ch <= 'Z') {
      return {uint32_t(NS_VK_A + (ch - 'A')), KEY_NAME_INDEX_USE_STRING, ch};
    }
    if (ch >= '0' && ch <= '9') {
      return {uint32_t(NS_VK_0 + (ch - '0')), KEY_NAME_INDEX_USE_STRING, ch};
    }
    if (ch == ' ') {
      return {NS_VK_SPACE, KEY_NAME_INDEX_USE_STRING, ch};
    }
    return {0, KEY_NAME_INDEX_USE_STRING, ch};
  }

  return {NS_VK_UNKNOWN, KEY_NAME_INDEX_Unidentified, 0};
}

static void FillKeyboardEvent(WidgetKeyboardEvent& aEvent,
                              const MinWaylandKeyInfo& aInfo,
                              const nsAString& aKeyValue,
                              uint32_t aModifiers) {
  aEvent.mKeyCode = aInfo.mKeyCode == NS_VK_UNKNOWN ? 0 : aInfo.mKeyCode;
  aEvent.mKeyNameIndex = aInfo.mKeyNameIndex;
  aEvent.mCodeNameIndex = CODE_NAME_INDEX_UNKNOWN;
  aEvent.mModifiers = aModifiers;
  if (aInfo.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    aEvent.mKeyValue.Assign(aInfo.mChar);
    if (aInfo.mChar) {
      aEvent.SetCharCode(aInfo.mChar);
    }
  }
  aEvent.AssignEventTime(WidgetEventTime());
}

static bool IsModifierKey(const MinWaylandKeyInfo& aInfo) {
  switch (aInfo.mKeyNameIndex) {
    case KEY_NAME_INDEX_Shift:
    case KEY_NAME_INDEX_Control:
    case KEY_NAME_INDEX_Alt:
    case KEY_NAME_INDEX_Meta:
      return true;
    default:
      return false;
  }
}

MinWaylandWindow::MinWaylandWindow() {
  mWidgetType = WidgetType::Native;
}

MinWaylandWindow::~MinWaylandWindow() { Destroy(); }

nsresult MinWaylandWindow::Create(nsIWidget* aParent,
                                  const LayoutDeviceIntRect& aRect,
                                  widget::InitData* aInitData) {
  nsresult rv = HeadlessWidget::Create(aParent, aRect, aInitData);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (mWindowType == WindowType::TopLevel || mWindowType == WindowType::Dialog ||
      mWindowType == WindowType::Popup) {
    mDisplay = MinWaylandDisplay::Get();
  }
  if (mWindowType == WindowType::Popup) {
    nsIWidget* parent = aParent;
    if (!parent || !parent->GetNativeData(NS_NATIVE_WINDOW)) {
      parent = GetTopLevelWidget();
    }
    if (parent && parent->GetNativeData(NS_NATIVE_WINDOW)) {
      mParentWindow = static_cast<MinWaylandWindow*>(parent);
    }
  }
  return NS_OK;
}

void MinWaylandWindow::Destroy() {
  DestroyNativeWindow();
  HeadlessWidget::Destroy();
}

void MinWaylandWindow::Show(bool aState) {
  HeadlessWidget::Show(aState);
  if (aState && mWindowType == WindowType::Popup) {
    MaybeShowPopup();
  } else if (aState) {
    EnsureNativeWindow();
  } else if (mWindowType == WindowType::Popup) {
    mPopupShowRetries = 0;
    DestroyNativeWindow();
  }
}

void MinWaylandWindow::Resize(double aWidth, double aHeight, bool aRepaint) {
  HeadlessWidget::Resize(aWidth, aHeight, aRepaint);
  if (mWindowType == WindowType::Popup && IsVisible() && !mSurface) {
    MaybeShowPopup();
  }
}

void MinWaylandWindow::Resize(double aX, double aY, double aWidth,
                              double aHeight, bool aRepaint) {
  HeadlessWidget::Resize(aX, aY, aWidth, aHeight, aRepaint);
  if (mWindowType == WindowType::Popup && IsVisible() && !mSurface) {
    MaybeShowPopup();
  }
}

nsresult MinWaylandWindow::SetTitle(const nsAString& aTitle) {
  mTitle = aTitle;
  if (mToplevel) {
    NS_ConvertUTF16toUTF8 title(mTitle);
    xdg_toplevel_set_title(mToplevel, title.get());
    mDisplay->Flush();
  }
  return NS_OK;
}

void* MinWaylandWindow::GetNativeData(uint32_t aDataType) {
  switch (aDataType) {
    case NS_NATIVE_WINDOW:
      return mSurface;
    default:
      return HeadlessWidget::GetNativeData(aDataType);
  }
}

bool MinWaylandWindow::EnsureNativeWindow() {
  if (mSurface) {
    return true;
  }
  if (!NS_IsMainThread()) {
    return false;
  }
  if (!mDisplay || !mDisplay->IsReady()) {
    LOG("Wayland display is not ready");
    return false;
  }

  mSurface = wl_compositor_create_surface(mDisplay->Compositor());
  if (!mSurface) {
    return false;
  }
  mDisplay->RegisterWindow(this);
  mXdgSurface = xdg_wm_base_get_xdg_surface(mDisplay->XdgWmBase(), mSurface);
  if (!mXdgSurface) {
    DestroyNativeWindow();
    return false;
  }
  xdg_surface_add_listener(mXdgSurface, &kXdgSurfaceListener, this);

  return mWindowType == WindowType::Popup ? EnsurePopupWindow()
                                          : EnsureToplevelWindow();
}

void MinWaylandWindow::MaybeShowPopup() {
  MOZ_ASSERT(mWindowType == WindowType::Popup);
  if (mSurface || !IsVisible()) {
    return;
  }
  LayoutDeviceIntSize size = GetClientSize();
  if (size.width <= 0 || size.height <= 0) {
    if (mPopupShowRetries++ < 20) {
      RefPtr self = this;
      NS_DelayedDispatchToCurrentThread(
          NS_NewRunnableFunction("MinWaylandWindow::MaybeShowPopup",
                                 [self] { self->MaybeShowPopup(); }),
          16);
    }
    return;
  }
  mPopupShowRetries = 0;
  EnsureNativeWindow();
}

bool MinWaylandWindow::EnsureToplevelWindow() {
  mToplevel = xdg_surface_get_toplevel(mXdgSurface);
  if (!mToplevel) {
    DestroyNativeWindow();
    return false;
  }
  xdg_toplevel_add_listener(mToplevel, &kXdgToplevelListener, this);
  xdg_toplevel_set_app_id(mToplevel, "waterfox-minwayland");
  if (!mTitle.IsEmpty()) {
    NS_ConvertUTF16toUTF8 title(mTitle);
    xdg_toplevel_set_title(mToplevel, title.get());
  } else {
    xdg_toplevel_set_title(mToplevel, "Waterfox");
  }
  xdg_toplevel_set_fullscreen(mToplevel, nullptr);
  wl_surface_commit(mSurface);
  wl_display_flush(mDisplay->Display());

  for (int i = 0; i < 8 && !mConfigured; i++) {
    wl_display_roundtrip(mDisplay->Display());
  }

  return mConfigured;
}

bool MinWaylandWindow::EnsurePopupWindow() {
  if (!mParentWindow || !mParentWindow->EnsureNativeWindow() ||
      !mParentWindow->mXdgSurface) {
    DestroyNativeWindow();
    return false;
  }

  LayoutDeviceIntSize size = BufferSize();
  xdg_positioner* positioner =
      xdg_wm_base_create_positioner(mDisplay->XdgWmBase());
  if (!positioner) {
    DestroyNativeWindow();
    return false;
  }
  auto destroyPositioner =
      MakeScopeExit([&] { xdg_positioner_destroy(positioner); });

  LayoutDeviceIntRect bounds = GetBounds();
  xdg_positioner_set_size(positioner, std::max(size.width, 1),
                          std::max(size.height, 1));
  xdg_positioner_set_anchor_rect(positioner, bounds.X(), bounds.Y(), 1, 1);
  xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
  xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
  xdg_positioner_set_constraint_adjustment(
      positioner, XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X |
                      XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y |
                      XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
                      XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);

  mPopup =
      xdg_surface_get_popup(mXdgSurface, mParentWindow->mXdgSurface, positioner);
  if (!mPopup) {
    DestroyNativeWindow();
    return false;
  }
  xdg_popup_add_listener(mPopup, &kXdgPopupListener, this);
  if (mDisplay->Seat() && mDisplay->LastInputSerial()) {
    xdg_popup_grab(mPopup, mDisplay->Seat(), mDisplay->LastInputSerial());
  }
  wl_surface_commit(mSurface);
  wl_display_flush(mDisplay->Display());

  for (int i = 0; i < 8 && !mConfigured; i++) {
    wl_display_roundtrip(mDisplay->Display());
  }
  return mConfigured;
}

LayoutDeviceIntSize MinWaylandWindow::BufferSize() {
  if (!mConfiguredSize.IsEmpty()) {
    return mConfiguredSize;
  }
  LayoutDeviceIntSize size = GetClientSize();
  if (size.width <= 0 || size.height <= 0) {
    return LayoutDeviceIntSize(800, 600);
  }
  return size;
}

UniquePtr<MinWaylandWindow::ShmBuffer> MinWaylandWindow::CreateBuffer(
    const LayoutDeviceIntSize& aSize) {
  if (!mDisplay || !mDisplay->IsReady() || aSize.width <= 0 ||
      aSize.height <= 0) {
    return nullptr;
  }

  auto buffer = MakeUnique<ShmBuffer>();
  buffer->mStride = aSize.width * 4;
  buffer->mLength = size_t(buffer->mStride) * size_t(aSize.height);
  buffer->mFd = CreateMemfd(buffer->mLength);
  if (buffer->mFd < 0) {
    return nullptr;
  }
  buffer->mData =
      mmap(nullptr, buffer->mLength, PROT_READ | PROT_WRITE, MAP_SHARED,
           buffer->mFd, 0);
  if (buffer->mData == MAP_FAILED) {
    return nullptr;
  }
  if (ShmBuffer* latest = LatestBuffer();
      latest && latest->mData != MAP_FAILED && latest->mLength == buffer->mLength) {
    memcpy(buffer->mData, latest->mData, buffer->mLength);
  }

  wl_shm_pool* pool =
      wl_shm_create_pool(mDisplay->Shm(), buffer->mFd, buffer->mLength);
  if (!pool) {
    return nullptr;
  }
  auto destroyPool = MakeScopeExit([&] { wl_shm_pool_destroy(pool); });
  buffer->mBuffer = wl_shm_pool_create_buffer(
      pool, 0, aSize.width, aSize.height, buffer->mStride,
      WL_SHM_FORMAT_XRGB8888);
  if (!buffer->mBuffer) {
    return nullptr;
  }
  wl_buffer_add_listener(buffer->mBuffer, &kBufferListener, buffer.get());

  buffer->mDrawTarget = gfx::Factory::CreateDrawTargetForData(
      gfx::BackendType::CAIRO, static_cast<uint8_t*>(buffer->mData),
      aSize.ToUnknownSize(), buffer->mStride, gfx::SurfaceFormat::B8G8R8X8);
  if (!buffer->mDrawTarget) {
    return nullptr;
  }

  return buffer;
}

MinWaylandWindow::ShmBuffer* MinWaylandWindow::LatestBuffer() const {
  if (mLiveBuffers.IsEmpty()) {
    return nullptr;
  }
  return mLiveBuffers.LastElement().get();
}

already_AddRefed<gfx::DrawTarget>
MinWaylandWindow::StartRemoteDrawingInRegion(
    const LayoutDeviceIntRegion& aInvalidRegion) {
  if (!mSurface) {
    if (!NS_IsMainThread()) {
      if (mWindowType == WindowType::Popup && IsVisible()) {
        RefPtr self = this;
        NS_DispatchToMainThread(NS_NewRunnableFunction(
            "MinWaylandWindow::MaybeShowPopup",
            [self] { self->MaybeShowPopup(); }));
      }
      return nullptr;
    }
    if (!EnsureNativeWindow()) {
      return nullptr;
    }
  }

  MutexAutoLock lock(mDisplay->WaylandMutex());
  mDrawingBuffer = CreateBuffer(BufferSize());
  if (!mDrawingBuffer) {
    return nullptr;
  }
  RefPtr<gfx::DrawTarget> target = mDrawingBuffer->mDrawTarget;
  return target.forget();
}

void MinWaylandWindow::EndRemoteDrawingInRegion(
    gfx::DrawTarget* aDrawTarget, const LayoutDeviceIntRegion& aInvalidRegion) {
  if (!mDrawingBuffer || mDrawingBuffer->mDrawTarget != aDrawTarget) {
    return;
  }

  mDrawingBuffer->mDrawTarget->Flush();
  MutexAutoLock lock(mDisplay->WaylandMutex());
  PresentBuffer(std::move(mDrawingBuffer), aInvalidRegion);
}

void MinWaylandWindow::PresentBuffer(
    UniquePtr<ShmBuffer> aBuffer, const LayoutDeviceIntRegion& aInvalidRegion) {
  if (!aBuffer || !mSurface) {
    return;
  }

  wl_surface_attach(mSurface, aBuffer->mBuffer, 0, 0);
  for (auto iter = aInvalidRegion.RectIter(); !iter.Done(); iter.Next()) {
    const LayoutDeviceIntRect& rect = iter.Get();
    wl_surface_damage(mSurface, rect.X(), rect.Y(), rect.Width(),
                      rect.Height());
  }
  wl_surface_commit(mSurface);
  wl_display_flush(mDisplay->Display());

  mLiveBuffers.AppendElement(std::move(aBuffer));
  PruneReleasedBuffers();
}

void MinWaylandWindow::PruneReleasedBuffers() {
  for (size_t i = mLiveBuffers.Length(); i > 0; i--) {
    if (mLiveBuffers[i - 1]->mReleased) {
      mLiveBuffers.RemoveElementAt(i - 1);
    }
  }
}

void MinWaylandWindow::CleanupRemoteDrawing() {
  if (!mDisplay) {
    return;
  }
  MutexAutoLock lock(mDisplay->WaylandMutex());
  mDrawingBuffer = nullptr;
  mLiveBuffers.Clear();
}

void MinWaylandWindow::DestroyNativeWindow() {
  if (!mDisplay) {
    return;
  }
  mDisplay->UnregisterWindow(this);
  MutexAutoLock lock(mDisplay->WaylandMutex());
  mDrawingBuffer = nullptr;
  mLiveBuffers.Clear();
  if (mPopup) {
    xdg_popup_destroy(mPopup);
    mPopup = nullptr;
  }
  if (mToplevel) {
    xdg_toplevel_destroy(mToplevel);
    mToplevel = nullptr;
  }
  if (mXdgSurface) {
    xdg_surface_destroy(mXdgSurface);
    mXdgSurface = nullptr;
  }
  if (mSurface) {
    wl_surface_destroy(mSurface);
    mSurface = nullptr;
  }
  wl_display_flush(mDisplay->Display());
  mConfigured = false;
}

void MinWaylandWindow::DispatchPointerEnter(const LayoutDeviceIntPoint& aPoint,
                                            uint32_t aModifiers) {
  WidgetMouseEvent event(true, eMouseEnterIntoWidget, this,
                         WidgetMouseEvent::eReal);
  event.mRefPoint = aPoint;
  event.mModifiers = aModifiers;
  event.AssignEventTime(WidgetEventTime());
  DispatchInputEvent(&event);
}

void MinWaylandWindow::DispatchPointerLeave(const LayoutDeviceIntPoint& aPoint,
                                            uint32_t aModifiers) {
  WidgetMouseEvent event(true, eMouseExitFromWidget, this,
                         WidgetMouseEvent::eReal);
  event.mRefPoint = aPoint;
  event.mModifiers = aModifiers;
  event.mExitFrom = Some(WidgetMouseEvent::ePlatformTopLevel);
  event.AssignEventTime(WidgetEventTime());
  DispatchInputEvent(&event);
}

void MinWaylandWindow::DispatchPointerMotion(const LayoutDeviceIntPoint& aPoint,
                                             uint32_t aModifiers) {
  WidgetMouseEvent event(true, eMouseMove, this, WidgetMouseEvent::eReal);
  event.mRefPoint = aPoint;
  event.mModifiers = aModifiers;
  event.AssignEventTime(WidgetEventTime());
  DispatchInputEvent(&event);
}

void MinWaylandWindow::DispatchPointerButton(EventMessage aMessage,
                                             MouseButton aButton,
                                             const LayoutDeviceIntPoint& aPoint,
                                             uint32_t aModifiers,
                                             uint16_t aButtons) {
  WidgetMouseEvent event(true, aMessage, this, WidgetMouseEvent::eReal);
  event.mRefPoint = aPoint;
  event.mButton = aButton;
  event.mButtons = aButtons;
  event.mClickCount = 1;
  event.mModifiers = aModifiers;
  event.AssignEventTime(WidgetEventTime());
  DispatchInputEvent(&event);

  if (aMessage == eMouseUp && aButton == MouseButton::eSecondary) {
    WidgetPointerEvent contextMenu(true, eContextMenu, this);
    contextMenu.mRefPoint = aPoint;
    contextMenu.mButton = MouseButton::eSecondary;
    contextMenu.mClickCount = 1;
    contextMenu.mModifiers = aModifiers;
    contextMenu.AssignEventTime(WidgetEventTime());
    DispatchInputEvent(&contextMenu);
  }
}

void MinWaylandWindow::DispatchPointerAxis(double aDeltaX, double aDeltaY,
                                           const LayoutDeviceIntPoint& aPoint,
                                           uint32_t aModifiers) {
  if (aDeltaX == 0.0 && aDeltaY == 0.0) {
    return;
  }
  WidgetWheelEvent event(true, eWheel, this);
  event.mDeltaMode = dom::WheelEvent_Binding::DOM_DELTA_PIXEL;
  event.mIsNoLineOrPageDelta = true;
  event.mDeltaX = aDeltaX;
  event.mDeltaY = aDeltaY;
  event.mRefPoint = aPoint;
  event.mModifiers = aModifiers;
  event.AssignEventTime(WidgetEventTime());
  DispatchInputEvent(&event);
}

void MinWaylandWindow::DispatchKeyboard(uint32_t aXkbKeycode,
                                        uint32_t aKeysym,
                                        const nsACString& aUtf8,
                                        bool aPressed,
                                        uint32_t aModifiers) {
  nsAutoString keyValue;
  CopyUTF8toUTF16(aUtf8, keyValue);
  MinWaylandKeyInfo keyInfo = KeyInfoFromKeysym(aKeysym, keyValue);
  WidgetKeyboardEvent keyEvent(true, aPressed ? eKeyDown : eKeyUp, this);
  FillKeyboardEvent(keyEvent, keyInfo, keyValue, aModifiers);
  RefPtr<TextEventDispatcher> dispatcher = GetTextEventDispatcher();
  if (dispatcher && NS_SUCCEEDED(dispatcher->BeginNativeInputTransaction())) {
    nsEventStatus status = nsEventStatus_eIgnore;
    dispatcher->DispatchKeyboardEvent(keyEvent.mMessage, keyEvent, status,
                                      nullptr);
  } else {
    DispatchInputEvent(&keyEvent);
  }

  if (!aPressed || IsModifierKey(keyInfo)) {
    return;
  }

  WidgetKeyboardEvent keyPress(true, eKeyPress, this);
  FillKeyboardEvent(keyPress, keyInfo, keyValue, aModifiers);
  if (keyInfo.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING &&
      keyInfo.mChar) {
    keyPress.mCharCode = keyInfo.mChar;
    keyPress.mKeyCode = 0;
  }
  if (dispatcher) {
    nsEventStatus status = nsEventStatus_eIgnore;
    if (NS_SUCCEEDED(dispatcher->BeginNativeInputTransaction())) {
      bool dispatched = dispatcher->MaybeDispatchKeypressEvents(keyPress, status);
      if (dispatched && status == nsEventStatus_eConsumeNoDefault) {
        return;
      }
      if (keyInfo.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING && keyInfo.mChar &&
          !(aModifiers & (MODIFIER_CONTROL | MODIFIER_ALT | MODIFIER_META)) &&
          NS_SUCCEEDED(dispatcher->BeginNativeInputTransaction())) {
        nsAutoString commitString;
        commitString.Assign(keyInfo.mChar);
        dispatcher->CommitComposition(status, &commitString);
      }
      return;
    }
  }
  DispatchInputEvent(&keyPress);
}

void MinWaylandWindow::XdgSurfaceConfigure(void* aData, xdg_surface* aSurface,
                                           uint32_t aSerial) {
  auto* self = static_cast<MinWaylandWindow*>(aData);
  xdg_surface_ack_configure(aSurface, aSerial);
  self->mConfigured = true;
}

void MinWaylandWindow::XdgToplevelConfigure(void* aData,
                                            xdg_toplevel* aToplevel,
                                            int32_t aWidth, int32_t aHeight,
                                            wl_array* aStates) {
  auto* self = static_cast<MinWaylandWindow*>(aData);
  if (aWidth > 0 && aHeight > 0) {
    LayoutDeviceIntSize size(aWidth, aHeight);
    self->mConfiguredSize = size;
    self->HeadlessWidget::Resize(aWidth, aHeight, true);
  }
}

void MinWaylandWindow::XdgToplevelClose(void* aData,
                                        xdg_toplevel* aToplevel) {}

void MinWaylandWindow::XdgPopupConfigure(void* aData, xdg_popup* aPopup,
                                         int32_t aX, int32_t aY,
                                         int32_t aWidth, int32_t aHeight) {
  auto* self = static_cast<MinWaylandWindow*>(aData);
  if (aWidth > 0 && aHeight > 0) {
    self->mConfiguredSize = LayoutDeviceIntSize(aWidth, aHeight);
    self->HeadlessWidget::Resize(aWidth, aHeight, true);
  }
}

void MinWaylandWindow::XdgPopupDone(void* aData, xdg_popup* aPopup) {
  auto* self = static_cast<MinWaylandWindow*>(aData);
  self->Show(false);
}

void MinWaylandWindow::XdgPopupRepositioned(void* aData, xdg_popup* aPopup,
                                            uint32_t aToken) {}

}  // namespace mozilla::widget
