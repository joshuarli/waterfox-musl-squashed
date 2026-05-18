/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MinWaylandServices.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/MediaControlKeySource.h"
#include "nsIFilePicker.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"

namespace mozilla::widget {

NS_IMPL_ISUPPORTS(MinWaylandFilePicker, nsIFilePicker)

NS_IMETHODIMP MinWaylandFilePicker::AppendFilter(const nsAString& aTitle,
                                                 const nsAString& aFilter) {
  return AppendRawFilter(aFilter);
}

NS_IMETHODIMP MinWaylandFilePicker::GetDefaultString(
    nsAString& aDefaultString) {
  aDefaultString = mDefaultString;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::SetDefaultString(
    const nsAString& aDefaultString) {
  mDefaultString = aDefaultString;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::GetDefaultExtension(
    nsAString& aDefaultExtension) {
  aDefaultExtension = mDefaultExtension;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::SetDefaultExtension(
    const nsAString& aDefaultExtension) {
  mDefaultExtension = aDefaultExtension;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::GetFile(nsIFile** aFile) {
  *aFile = nullptr;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::GetFileURL(nsIURI** aFileURL) {
  *aFileURL = nullptr;
  return NS_OK;
}

NS_IMETHODIMP MinWaylandFilePicker::Open(
    nsIFilePickerShownCallback* aCallback) {
  if (aCallback) {
    NS_DispatchToCurrentThread(NS_NewRunnableFunction(
        "MinWaylandFilePicker::Open",
        [callback = nsCOMPtr<nsIFilePickerShownCallback>(aCallback)] {
          callback->Done(nsIFilePicker::returnCancel);
        }));
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(MinWaylandColorPicker, nsIColorPicker)

nsresult MinWaylandColorPicker::InitNative(
    const nsTArray<nsString>& aDefaultColors) {
  return NS_OK;
}

nsresult MinWaylandColorPicker::OpenNative() {
  if (mCallback) {
    NS_DispatchToCurrentThread(NS_NewRunnableFunction(
        "MinWaylandColorPicker::OpenNative",
        [callback = nsCOMPtr<nsIColorPickerShownCallback>(mCallback)] {
          callback->Done(EmptyString());
        }));
  }
  return NS_OK;
}

nsresult MinWaylandDragSession::InvokeDragSessionImpl(
    nsIWidget* aWidget, nsIArray* aTransferableArray,
    const Maybe<CSSIntRegion>& aRegion, uint32_t aActionType) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

static StaticRefPtr<MinWaylandDragService> sDragService;

already_AddRefed<MinWaylandDragService> MinWaylandDragService::GetInstance() {
  if (!sDragService) {
    sDragService = new MinWaylandDragService();
    ClearOnShutdown(&sDragService);
  }
  return do_AddRef(sDragService);
}

already_AddRefed<nsIDragSession> MinWaylandDragService::CreateDragSession() {
  RefPtr<nsIDragSession> session = new MinWaylandDragSession();
  return session.forget();
}

static StaticRefPtr<MinWaylandUserIdleService> sUserIdleService;

already_AddRefed<MinWaylandUserIdleService>
MinWaylandUserIdleService::GetInstance() {
  if (!sUserIdleService) {
    sUserIdleService = new MinWaylandUserIdleService();
    ClearOnShutdown(&sUserIdleService);
  }
  return do_AddRef(sUserIdleService);
}

bool MinWaylandUserIdleService::PollIdleTime(uint32_t* aIdleTime) {
  return false;
}

class MinWaylandMediaControlKeySource final
    : public dom::MediaControlKeySource {
 public:
  NS_INLINE_DECL_REFCOUNTING(MinWaylandMediaControlKeySource, override)

  bool Open() override { return false; }
  bool IsOpened() const override { return false; }
  void SetSupportedMediaKeys(const MediaKeysArray& aSupportedKeys) override {}

 private:
  ~MinWaylandMediaControlKeySource() override = default;
};

dom::MediaControlKeySource* CreateMediaControlKeySource() {
  return new MinWaylandMediaControlKeySource();
}

}  // namespace mozilla::widget
