// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
 
#pragma once
// Scanner Net API Module

/*
    距離と計測方位のデータを受け取り、ネットワーク越しに外部プログラムにデータを提供する機能パッケージ
    UE内部データ表現からセンサハードウェア上でのデータ表現への変換を担当する
    これには座標系(Yaw方向, Pitch方向等)の変換を含む
*/

#include "CoreMinimal.h"
#include "Networking.h"
#include "Components/ActorComponent.h"
#include "Comm/Comm.h"
#include "Comm/ActorCommMgrAPI.h"
#include <array>
#include "ScannerIOProtocol.generated.h"

UCLASS(ClassGroup = "Sensors", BluePrintable)
class UScannerIOProtocol : public UActorComponent, public IActorCommListener {
  GENERATED_BODY()

protected:
  TArray<int> Ranges;
  TArray<uint8_t> Intensities;
  TArray<float> Yaws;
  bool InRange = false;
  float LastYaw = -180;
  class UScanStrategy* Scanner=nullptr;
public:
  virtual void BeginPlay() override;

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) { Super::EndPlay(EndPlayReason); };

  virtual void pushScan(TArray<float> &ranges, TArray<float> &intensities, TArray<FRotator> &dirs, TArray<float> &timestamps);

  virtual void sendPacket() {};

  virtual bool setRemoteIP(const FString &remoteIP) { return false; };

  virtual void RemoteAddressChanged(const FString& Address) override {
      setRemoteIP(Address);
  };

  float EndHAngle;     // scan end angle [deg]
  float StartHAngle;     // scan end angle [deg]
};

UCLASS(ClassGroup = "Sensors", BluePrintable)
class UScannerIOProtocolUDP : public UScannerIOProtocol {
  GENERATED_BODY()

protected:
    FSocket *Socket = nullptr;
public:
  virtual void BeginPlay() override;

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;

  virtual void sendPacket()override ;

  bool setRemoteIP(const FString &remoteIP) override;

  UPROPERTY(EditAnywhere)
    int RemotePort = 65432;     // Remote Port Number
  UPROPERTY(EditAnywhere)
    FString RemoteIP = "255.255.255.255";
  UPROPERTY(EditAnywhere)
    bool ForceBroadcast=false;
  TSharedPtr<FInternetAddr> RemoteAddr;
  uint32 BroadcastIP;
  bool RemoteIsBroadcast = true;
};

UCLASS(ClassGroup = "Sensors", BluePrintable)
class UScannerIOProtocolJsonComm : public UScannerIOProtocol {
  GENERATED_BODY()

public:
  virtual void BeginPlay() override;

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;

  virtual void sendPacket()override;

  CommEndpointOut Comm;
};

UCLASS(ClassGroup = "Sensors", BluePrintable)
class UScannerIOProtocolVelodyne : public UScannerIOProtocolUDP {
  GENERATED_BODY()

  FBufferArchive Pack;
  size_t NDataBlocks = 0;

  TArray<uint16_t> Rangedata;
  TArray<uint8_t> Intensity;
  uint16_t Azimuth;
  float Timestamp;

public:
  virtual void BeginPlay() override;

  virtual void sendPacket()override;

  void PreparePacket();

  virtual void pushScan(TArray<float> &ranges, TArray<float> &intensities, TArray<FRotator> &dirs,TArray<float> &timestamps) override;

  void pushRange(TArray<uint16_t> &rangedata, TArray<float> & ranges, size_t pos);

  // Data granularity[mm] in packets. VLP-16: 2mm, VLP-32C: 4mm
  UPROPERTY(EditAnywhere, Category = "Setup")
    float UnitRange=2;
  // ModelID vlp-16: 0x22(34), hdl-32: 0x21(33), vlp-32c: 0x28(40), vls-128: 0xa1(161)
  UPROPERTY(EditAnywhere, Category = "Setup")
    uint8 ModelID = 0x22;
  UPROPERTY(EditAnywhere, Category = "Setup")
  int IntensityDRange = 255;
};
