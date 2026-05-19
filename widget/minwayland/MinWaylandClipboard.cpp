/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MinWaylandClipboard.h"

#include "MinWaylandDisplay.h"
#include "mozilla/ScopeExit.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsITransferable.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <utility>

namespace mozilla::widget {

static constexpr auto kTextPlainUtf8 = "text/plain;charset=utf-8";
static constexpr auto kUtf8String = "UTF8_STRING";

struct MinWaylandClipboard::DataOffer {
  explicit DataOffer(wl_data_offer* aOffer) : mOffer(aOffer) {}

  ~DataOffer() {
    if (mOffer) {
      wl_data_offer_destroy(mOffer);
    }
  }

  bool HasMime(const nsACString& aMimeType) const {
    return mMimeTypes.Contains(aMimeType);
  }

  const char* BestTextMime() const {
    if (HasMime(nsDependentCString(kTextPlainUtf8))) {
      return kTextPlainUtf8;
    }
    if (HasMime(nsLiteralCString(kTextMime))) {
      return kTextMime;
    }
    if (HasMime(nsDependentCString(kUtf8String))) {
      return kUtf8String;
    }
    return nullptr;
  }

  wl_data_offer* mOffer = nullptr;
  nsTArray<nsCString> mMimeTypes;
};

struct MinWaylandClipboard::ClipboardSource {
  ClipboardSource(MinWaylandClipboard* aOwner, wl_data_source* aSource,
                  const nsACString& aText)
      : mOwner(aOwner), mSource(aSource), mText(aText) {}

  ~ClipboardSource() {
    if (mSource) {
      wl_data_source_destroy(mSource);
    }
  }

  MinWaylandClipboard* mOwner = nullptr;
  wl_data_source* mSource = nullptr;
  nsCString mText;
};

static const wl_data_offer_listener kDataOfferListener = {
    MinWaylandClipboard::DataOfferOffer,
    nullptr,
    nullptr,
};

static const wl_data_device_listener kDataDeviceListener = {
    MinWaylandClipboard::DataDeviceDataOffer,
    MinWaylandClipboard::DataDeviceEnter,
    MinWaylandClipboard::DataDeviceLeave,
    MinWaylandClipboard::DataDeviceMotion,
    MinWaylandClipboard::DataDeviceDrop,
    MinWaylandClipboard::DataDeviceSelection,
};

static const wl_data_source_listener kDataSourceListener = {
    MinWaylandClipboard::DataSourceTarget,
    MinWaylandClipboard::DataSourceSend,
    MinWaylandClipboard::DataSourceCancelled,
    MinWaylandClipboard::DataSourceDndDropPerformed,
    MinWaylandClipboard::DataSourceDndFinished,
    MinWaylandClipboard::DataSourceAction,
};

NS_IMPL_ISUPPORTS_INHERITED0(MinWaylandClipboard, nsBaseClipboard)

MinWaylandClipboard::MinWaylandClipboard()
    : nsBaseClipboard(dom::ClipboardCapabilities(
          true /* supportsSelectionClipboard */,
          true /* supportsFindClipboard */,
          true /* supportsSelectionCache */)),
      mDisplay(MinWaylandDisplay::Get()) {
  for (auto& clipboard : mClipboards) {
    clipboard = MakeUnique<HeadlessClipboardData>();
  }
  EnsureDataDevice();
}

MinWaylandClipboard::~MinWaylandClipboard() {
  ClearSelectionOffer();
  mPendingOffers.Clear();
  mSource = nullptr;
  if (mDataDevice) {
    wl_data_device_destroy(mDataDevice);
  }
}

bool MinWaylandClipboard::EnsureDataDevice() {
  if (mDataDevice) {
    return true;
  }
  if (!mDisplay || !mDisplay->Display() || !mDisplay->DataDeviceManager() ||
      !mDisplay->Seat()) {
    return false;
  }
  mDataDevice = wl_data_device_manager_get_data_device(
      mDisplay->DataDeviceManager(), mDisplay->Seat());
  if (!mDataDevice) {
    return false;
  }
  wl_data_device_add_listener(mDataDevice, &kDataDeviceListener, this);
  mDisplay->Roundtrip();
  return true;
}

NS_IMETHODIMP
MinWaylandClipboard::SetNativeClipboardData(nsITransferable* aTransferable,
                                            ClipboardType aWhichClipboard) {
  MOZ_DIAGNOSTIC_ASSERT(aTransferable);
  MOZ_DIAGNOSTIC_ASSERT(
      nsIClipboard::IsClipboardTypeSupported(aWhichClipboard));

  EmptyNativeClipboardData(aWhichClipboard);

  nsTArray<nsCString> flavors;
  nsresult rv = aTransferable->FlavorsTransferableCanExport(flavors);
  if (NS_FAILED(rv)) {
    return rv;
  }

  auto& clipboard = mClipboards[aWhichClipboard];
  MOZ_ASSERT(clipboard);

  for (const auto& flavor : flavors) {
    if (!flavor.EqualsLiteral(kTextMime) && !flavor.EqualsLiteral(kHTMLMime)) {
      continue;
    }

    nsCOMPtr<nsISupports> data;
    rv = aTransferable->GetTransferData(flavor.get(), getter_AddRefs(data));
    if (NS_FAILED(rv)) {
      continue;
    }

    nsCOMPtr<nsISupportsString> wideString = do_QueryInterface(data);
    if (!wideString) {
      continue;
    }

    nsAutoString text;
    wideString->GetData(text);
    flavor.EqualsLiteral(kTextMime) ? clipboard->SetText(text)
                                    : clipboard->SetHTML(text);
  }

  if (aWhichClipboard == kGlobalClipboard) {
    PublishGlobalSelection();
  }

  return NS_OK;
}

void MinWaylandClipboard::PublishGlobalSelection() {
  auto& clipboard = mClipboards[kGlobalClipboard];
  if (!clipboard || !clipboard->HasText() || !EnsureDataDevice()) {
    return;
  }

  wl_data_source* source =
      wl_data_device_manager_create_data_source(mDisplay->DataDeviceManager());
  if (!source) {
    return;
  }

  NS_ConvertUTF16toUTF8 text(clipboard->GetText());
  auto newSource = MakeUnique<ClipboardSource>(this, source, text);
  wl_data_source_add_listener(source, &kDataSourceListener, newSource.get());
  wl_data_source_offer(source, kTextPlainUtf8);
  wl_data_source_offer(source, kTextMime);
  wl_data_source_offer(source, kUtf8String);

  mSource = std::move(newSource);
  wl_data_device_set_selection(mDataDevice, source, mDisplay->LastInputSerial());
  mDisplay->Roundtrip();
}

already_AddRefed<nsISupports> MinWaylandClipboard::ToSupportsString(
    const nsAString& aText) {
  nsresult rv;
  nsCOMPtr<nsISupportsString> dataWrapper =
      do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv)) || !dataWrapper) {
    return nullptr;
  }
  rv = dataWrapper->SetData(aText);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }
  return dataWrapper.forget().downcast<nsISupports>();
}

Result<nsCOMPtr<nsISupports>, nsresult>
MinWaylandClipboard::GetNativeClipboardData(const nsACString& aFlavor,
                                            ClipboardType aWhichClipboard) {
  MOZ_DIAGNOSTIC_ASSERT(
      nsIClipboard::IsClipboardTypeSupported(aWhichClipboard));

  if (!aFlavor.EqualsLiteral(kTextMime) && !aFlavor.EqualsLiteral(kHTMLMime)) {
    return nsCOMPtr<nsISupports>{};
  }

  auto& clipboard = mClipboards[aWhichClipboard];
  MOZ_ASSERT(clipboard);
  const bool isText = aFlavor.EqualsLiteral(kTextMime);

  if (aWhichClipboard == kGlobalClipboard && isText && !mSource) {
    if (Maybe<nsCString> externalText = ReadSelectionText()) {
      nsAutoString text;
      CopyUTF8toUTF16(*externalText, text);
      clipboard->SetText(text);
    }
  }

  if (!(isText ? clipboard->HasText() : clipboard->HasHTML())) {
    return nsCOMPtr<nsISupports>{};
  }

  nsCOMPtr<nsISupports> result =
      ToSupportsString(isText ? clipboard->GetText() : clipboard->GetHTML());
  return result;
}

Maybe<nsCString> MinWaylandClipboard::ReadSelectionText() {
  if (!EnsureDataDevice()) {
    return Nothing();
  }
  if (!mSelectionOffer) {
    mDisplay->Roundtrip();
  }
  if (!mSelectionOffer) {
    return Nothing();
  }

  const char* mime = mSelectionOffer->BestTextMime();
  if (!mime) {
    return Nothing();
  }

  int pipeFds[2];
  if (pipe(pipeFds) != 0) {
    return Nothing();
  }
  fcntl(pipeFds[0], F_SETFD, FD_CLOEXEC);
  fcntl(pipeFds[1], F_SETFD, FD_CLOEXEC);
  auto closeRead = MakeScopeExit([&] { close(pipeFds[0]); });
  auto closeWrite = MakeScopeExit([&] { close(pipeFds[1]); });

  wl_data_offer_receive(mSelectionOffer->mOffer, mime, pipeFds[1]);
  mDisplay->Flush();
  closeWrite.release();
  close(pipeFds[1]);

  nsCString data;
  char buffer[4096];
  while (true) {
    pollfd pfd = {pipeFds[0], POLLIN, 0};
    int pollResult;
    do {
      pollResult = poll(&pfd, 1, 2000);
    } while (pollResult < 0 && errno == EINTR);

    if (pollResult <= 0 || !(pfd.revents & (POLLIN | POLLHUP))) {
      break;
    }

    ssize_t readResult;
    do {
      readResult = read(pipeFds[0], buffer, sizeof(buffer));
    } while (readResult < 0 && errno == EINTR);

    if (readResult > 0) {
      data.Append(buffer, readResult);
      continue;
    }
    break;
  }

  if (data.IsEmpty()) {
    return Nothing();
  }
  return Some(data);
}

nsresult MinWaylandClipboard::EmptyNativeClipboardData(
    ClipboardType aWhichClipboard) {
  MOZ_DIAGNOSTIC_ASSERT(
      nsIClipboard::IsClipboardTypeSupported(aWhichClipboard));
  auto& clipboard = mClipboards[aWhichClipboard];
  MOZ_ASSERT(clipboard);
  clipboard->Clear();
  if (aWhichClipboard == kGlobalClipboard) {
    mSource = nullptr;
  }
  return NS_OK;
}

Result<int32_t, nsresult> MinWaylandClipboard::GetNativeClipboardSequenceNumber(
    ClipboardType aWhichClipboard) {
  MOZ_DIAGNOSTIC_ASSERT(
      nsIClipboard::IsClipboardTypeSupported(aWhichClipboard));
  auto& clipboard = mClipboards[aWhichClipboard];
  MOZ_ASSERT(clipboard);
  return clipboard->GetChangeCount();
}

bool MinWaylandClipboard::HasExternalText() const {
  return mSelectionOffer && mSelectionOffer->BestTextMime();
}

Result<bool, nsresult> MinWaylandClipboard::HasNativeClipboardDataMatchingFlavors(
    const nsTArray<nsCString>& aFlavorList, ClipboardType aWhichClipboard) {
  MOZ_DIAGNOSTIC_ASSERT(
      nsIClipboard::IsClipboardTypeSupported(aWhichClipboard));

  auto& clipboard = mClipboards[aWhichClipboard];
  MOZ_ASSERT(clipboard);

  for (const auto& flavor : aFlavorList) {
    if (flavor.EqualsLiteral(kTextMime) &&
        (clipboard->HasText() ||
         (aWhichClipboard == kGlobalClipboard && HasExternalText()))) {
      return true;
    }
    if (flavor.EqualsLiteral(kHTMLMime) && clipboard->HasHTML()) {
      return true;
    }
  }
  return false;
}

void MinWaylandClipboard::ClearSelectionOffer() { mSelectionOffer = nullptr; }

void MinWaylandClipboard::SourceCancelled(ClipboardSource* aSource) {
  if (mSource.get() == aSource) {
    mSource = nullptr;
  }
}

void MinWaylandClipboard::DataOfferOffer(void* aData, wl_data_offer* aOffer,
                                         const char* aMimeType) {
  auto* offer = static_cast<DataOffer*>(aData);
  offer->mMimeTypes.AppendElement(nsDependentCString(aMimeType));
}

void MinWaylandClipboard::DataDeviceDataOffer(void* aData,
                                              wl_data_device* aDevice,
                                              wl_data_offer* aOffer) {
  auto* self = static_cast<MinWaylandClipboard*>(aData);
  auto offer = MakeUnique<DataOffer>(aOffer);
  wl_data_offer_add_listener(aOffer, &kDataOfferListener, offer.get());
  self->mPendingOffers.AppendElement(std::move(offer));
}

void MinWaylandClipboard::DataDeviceEnter(void* aData, wl_data_device* aDevice,
                                          uint32_t aSerial,
                                          wl_surface* aSurface, wl_fixed_t aX,
                                          wl_fixed_t aY,
                                          wl_data_offer* aOffer) {}

void MinWaylandClipboard::DataDeviceLeave(void* aData,
                                          wl_data_device* aDevice) {}

void MinWaylandClipboard::DataDeviceMotion(void* aData,
                                           wl_data_device* aDevice,
                                           uint32_t aTime, wl_fixed_t aX,
                                           wl_fixed_t aY) {}

void MinWaylandClipboard::DataDeviceDrop(void* aData,
                                         wl_data_device* aDevice) {}

void MinWaylandClipboard::DataDeviceSelection(void* aData,
                                              wl_data_device* aDevice,
                                              wl_data_offer* aOffer) {
  auto* self = static_cast<MinWaylandClipboard*>(aData);
  self->ClearSelectionOffer();
  if (!aOffer) {
    return;
  }
  for (size_t i = 0; i < self->mPendingOffers.Length(); i++) {
    if (self->mPendingOffers[i]->mOffer == aOffer) {
      self->mSelectionOffer = std::move(self->mPendingOffers[i]);
      self->mPendingOffers.RemoveElementAt(i);
      return;
    }
  }
}

void MinWaylandClipboard::DataSourceTarget(void* aData,
                                           wl_data_source* aSource,
                                           const char* aMimeType) {}

void MinWaylandClipboard::DataSourceSend(void* aData, wl_data_source* aSource,
                                         const char* aMimeType, int32_t aFd) {
  auto* source = static_cast<ClipboardSource*>(aData);
  auto closeFd = MakeScopeExit([&] { close(aFd); });
  if (!source) {
    return;
  }

  size_t offset = 0;
  while (offset < source->mText.Length()) {
    ssize_t written = write(aFd, source->mText.Data() + offset,
                            source->mText.Length() - offset);
    if (written > 0) {
      offset += written;
      continue;
    }
    if (written < 0 && errno == EINTR) {
      continue;
    }
    break;
  }
}

void MinWaylandClipboard::DataSourceCancelled(void* aData,
                                              wl_data_source* aSource) {
  auto* source = static_cast<ClipboardSource*>(aData);
  if (source && source->mOwner) {
    source->mOwner->SourceCancelled(source);
  }
}

void MinWaylandClipboard::DataSourceDndDropPerformed(void* aData,
                                                     wl_data_source* aSource) {}

void MinWaylandClipboard::DataSourceDndFinished(void* aData,
                                                wl_data_source* aSource) {}

void MinWaylandClipboard::DataSourceAction(void* aData, wl_data_source* aSource,
                                           uint32_t aDndAction) {}

}  // namespace mozilla::widget
