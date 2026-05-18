/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GfxInfo.h"

#include "GfxDriverInfo.h"
#include "nsIGfxInfo.h"

namespace mozilla::widget {

nsresult GfxInfo::Init() { return GfxInfoBase::Init(); }

NS_IMETHODIMP GfxInfo::GetD2DEnabled(bool* aD2DEnabled) {
  *aD2DEnabled = false;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetDWriteEnabled(bool* aDWriteEnabled) {
  *aDWriteEnabled = false;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetDWriteVersion(nsAString& aDwriteVersion) {
  aDwriteVersion.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetEmbeddedInFirefoxReality(
    bool* aEmbeddedInFirefoxReality) {
  *aEmbeddedInFirefoxReality = false;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetHasBattery(bool* aHasBattery) {
  *aHasBattery = false;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetCleartypeParameters(nsAString& aCleartypeParams) {
  aCleartypeParams.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetWindowProtocol(nsAString& aWindowProtocol) {
  aWindowProtocol.AssignLiteral("wayland");
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetTestType(nsAString& aTestType) {
  aTestType.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDescription(nsAString& aAdapterDescription) {
  aAdapterDescription.AssignLiteral("minwayland");
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriver(nsAString& aAdapterDriver) {
  aAdapterDriver.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterVendorID(nsAString& aAdapterVendorID) {
  aAdapterVendorID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDeviceID(nsAString& aAdapterDeviceID) {
  aAdapterDeviceID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterSubsysID(nsAString& aAdapterSubsysID) {
  aAdapterSubsysID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterRAM(uint32_t* aAdapterRAM) {
  *aAdapterRAM = 0;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverVendor(
    nsAString& aAdapterDriverVendor) {
  aAdapterDriverVendor.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverVersion(
    nsAString& aAdapterDriverVersion) {
  aAdapterDriverVersion.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverDate(nsAString& aAdapterDriverDate) {
  aAdapterDriverDate.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDescription2(nsAString& aAdapterDescription) {
  aAdapterDescription.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriver2(nsAString& aAdapterDriver) {
  aAdapterDriver.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterVendorID2(nsAString& aAdapterVendorID) {
  aAdapterVendorID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDeviceID2(nsAString& aAdapterDeviceID) {
  aAdapterDeviceID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterSubsysID2(nsAString& aAdapterSubsysID) {
  aAdapterSubsysID.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterRAM2(uint32_t* aAdapterRAM) {
  *aAdapterRAM = 0;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverVendor2(
    nsAString& aAdapterDriverVendor) {
  aAdapterDriverVendor.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverVersion2(
    nsAString& aAdapterDriverVersion) {
  aAdapterDriverVersion.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetAdapterDriverDate2(nsAString& aAdapterDriverDate) {
  aAdapterDriverDate.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetIsGPU2Active(bool* aIsGPU2Active) {
  *aIsGPU2Active = false;
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::GetDrmRenderDevice(nsACString& aDrmRenderDevice) {
  aDrmRenderDevice.Truncate();
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::SpoofVendorID(const nsAString& aVendorID) {
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::SpoofDeviceID(const nsAString& aDeviceID) {
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::SpoofDriverVersion(
    const nsAString& aDriverVersion) {
  return NS_OK;
}

NS_IMETHODIMP GfxInfo::SpoofOSVersion(uint32_t aVersion) { return NS_OK; }

const nsTArray<GfxDriverInfo>& GfxInfo::GetGfxDriverInfo() {
  if (!sDriverInfo) {
    sDriverInfo = new nsTArray<GfxDriverInfo>();
  }
  return *sDriverInfo;
}

nsresult GfxInfo::GetFeatureStatusImpl(
    int32_t aFeature, int32_t* aStatus, nsAString& aSuggestedDriverVersion,
    const nsTArray<GfxDriverInfo>& aDriverInfo, nsACString& aFailureId,
    OperatingSystem* aOS) {
  *aStatus = nsIGfxInfo::FEATURE_STATUS_OK;
  return NS_OK;
}

}  // namespace mozilla::widget
