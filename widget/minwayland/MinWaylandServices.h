/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_minwayland_MinWaylandServices_h
#define widget_minwayland_MinWaylandServices_h

#include "nsBaseColorPicker.h"
#include "nsBaseDragService.h"
#include "nsBaseFilePicker.h"
#include "nsUserIdleService.h"

namespace mozilla::widget {

class MinWaylandFilePicker final : public nsBaseFilePicker {
 public:
  NS_DECL_ISUPPORTS
  NS_IMETHOD AppendFilter(const nsAString& aTitle,
                          const nsAString& aFilter) override;
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString) override;
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString) override;
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension) override;
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension) override;
  NS_IMETHOD GetFile(nsIFile** aFile) override;
  NS_IMETHOD GetFileURL(nsIURI** aFileURL) override;
  NS_IMETHOD Open(nsIFilePickerShownCallback* aCallback) override;

 protected:
  ~MinWaylandFilePicker() override = default;
  void InitNative(nsIWidget* aParent, const nsAString& aTitle) override {}

 private:
  nsString mDefaultString;
  nsString mDefaultExtension;
};

class MinWaylandColorPicker final : public nsBaseColorPicker {
 public:
  NS_DECL_ISUPPORTS

 protected:
  ~MinWaylandColorPicker() override = default;
  nsresult InitNative(const nsTArray<nsString>& aDefaultColors) override;
  nsresult OpenNative() override;
};

class MinWaylandDragSession final : public nsBaseDragSession {
 public:
  NS_INLINE_DECL_REFCOUNTING_INHERITED(MinWaylandDragSession,
                                       nsBaseDragSession)

 protected:
  ~MinWaylandDragSession() override = default;

  MOZ_CAN_RUN_SCRIPT nsresult InvokeDragSessionImpl(
      nsIWidget* aWidget, nsIArray* aTransferableArray,
      const Maybe<CSSIntRegion>& aRegion, uint32_t aActionType) override;
};

class MinWaylandDragService final : public nsBaseDragService {
 public:
  NS_INLINE_DECL_REFCOUNTING_INHERITED(MinWaylandDragService, nsBaseDragService)

  static already_AddRefed<MinWaylandDragService> GetInstance();

 protected:
  ~MinWaylandDragService() override = default;

  already_AddRefed<nsIDragSession> CreateDragSession() override;
};

class MinWaylandUserIdleService final : public nsUserIdleService {
 public:
  NS_INLINE_DECL_REFCOUNTING_INHERITED(MinWaylandUserIdleService,
                                       nsUserIdleService)

  static already_AddRefed<MinWaylandUserIdleService> GetInstance();
  bool PollIdleTime(uint32_t* aIdleTime) override;

 protected:
  ~MinWaylandUserIdleService() override = default;
};

}  // namespace mozilla::widget

#endif
