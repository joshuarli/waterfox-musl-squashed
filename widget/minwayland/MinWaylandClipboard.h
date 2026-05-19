/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef widget_minwayland_MinWaylandClipboard_h
#define widget_minwayland_MinWaylandClipboard_h

#include "HeadlessClipboardData.h"
#include "nsBaseClipboard.h"
#include "nsIClipboard.h"
#include "mozilla/Maybe.h"
#include "mozilla/Result.h"
#include "mozilla/UniquePtr.h"

#include <wayland-client.h>

namespace mozilla::widget {

class MinWaylandDisplay;

class MinWaylandClipboard final : public nsBaseClipboard {
 public:
  MinWaylandClipboard();

  NS_DECL_ISUPPORTS_INHERITED

  Result<int32_t, nsresult> GetNativeClipboardSequenceNumber(
      ClipboardType aWhichClipboard) override;

  static void DataOfferOffer(void* aData, wl_data_offer* aOffer,
                             const char* aMimeType);
  static void DataDeviceDataOffer(void* aData, wl_data_device* aDevice,
                                  wl_data_offer* aOffer);
  static void DataDeviceEnter(void* aData, wl_data_device* aDevice,
                              uint32_t aSerial, wl_surface* aSurface,
                              wl_fixed_t aX, wl_fixed_t aY,
                              wl_data_offer* aOffer);
  static void DataDeviceLeave(void* aData, wl_data_device* aDevice);
  static void DataDeviceMotion(void* aData, wl_data_device* aDevice,
                               uint32_t aTime, wl_fixed_t aX, wl_fixed_t aY);
  static void DataDeviceDrop(void* aData, wl_data_device* aDevice);
  static void DataDeviceSelection(void* aData, wl_data_device* aDevice,
                                  wl_data_offer* aOffer);
  static void DataSourceTarget(void* aData, wl_data_source* aSource,
                               const char* aMimeType);
  static void DataSourceSend(void* aData, wl_data_source* aSource,
                             const char* aMimeType, int32_t aFd);
  static void DataSourceCancelled(void* aData, wl_data_source* aSource);
  static void DataSourceDndDropPerformed(void* aData, wl_data_source* aSource);
  static void DataSourceDndFinished(void* aData, wl_data_source* aSource);
  static void DataSourceAction(void* aData, wl_data_source* aSource,
                               uint32_t aDndAction);

 protected:
  ~MinWaylandClipboard() override;

  NS_IMETHOD SetNativeClipboardData(nsITransferable* aTransferable,
                                    ClipboardType aWhichClipboard) override;
  Result<nsCOMPtr<nsISupports>, nsresult> GetNativeClipboardData(
      const nsACString& aFlavor, ClipboardType aWhichClipboard) override;
  nsresult EmptyNativeClipboardData(ClipboardType aWhichClipboard) override;
  Result<bool, nsresult> HasNativeClipboardDataMatchingFlavors(
      const nsTArray<nsCString>& aFlavorList,
      ClipboardType aWhichClipboard) override;

 private:
  struct DataOffer;
  struct ClipboardSource;

  bool EnsureDataDevice();
  void PublishGlobalSelection();
  Maybe<nsCString> ReadSelectionText();
  bool HasExternalText() const;
  void ClearSelectionOffer();
  void SourceCancelled(ClipboardSource* aSource);
  already_AddRefed<nsISupports> ToSupportsString(const nsAString& aText);

  MinWaylandDisplay* mDisplay = nullptr;
  wl_data_device* mDataDevice = nullptr;
  UniquePtr<ClipboardSource> mSource;
  nsTArray<UniquePtr<DataOffer>> mPendingOffers;
  UniquePtr<DataOffer> mSelectionOffer;
  UniquePtr<HeadlessClipboardData>
      mClipboards[nsIClipboard::kClipboardTypeCount];
};

}  // namespace mozilla::widget

#endif
