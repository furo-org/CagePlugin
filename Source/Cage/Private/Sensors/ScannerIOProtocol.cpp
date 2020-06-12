// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ScannerIOProtocol.h"
#include "ScanStrategy.h"
#include "LIDAR.h"
#include "Comm/Types.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"
#include"Runtime/Core/Public/HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarBroadcast(
  TEXT("cage.Lidar.Broadcast"),
  0,
  TEXT("Enable broadcasting packets when no destination is specified.\n")
  TEXT(" 0: Disable broadcast\n")
  TEXT(" 1: Enable broadcast"),
  ECVF_RenderThreadSafe
);

void UScannerIOProtocol::BeginPlay()
{
  Super::BeginPlay();
  Scanner = GetOwner()->FindComponentByClass<UScanStrategy>();
  if (Scanner != nullptr){
    UE_LOG(LogTemp, Warning, TEXT("Scanner Component found"));
  }
  else {
    UE_LOG(LogTemp, Warning, TEXT("No Scanner Component found"));
  }
  StartHAngle = CastChecked<ALidar>(GetOwner())->StartHAngle;
  EndHAngle = CastChecked<ALidar>(GetOwner())->EndHAngle;
}

void UScannerIOProtocol::pushScan(TArray<float> &ranges, TArray<float> &intensities, TArray<FRotator> &dirs, TArray<float> &timestamps)
{
  if (!ensure(Scanner != nullptr)) return;

  if (ranges.Num() != dirs.Num())
    UE_LOG(LogTemp, Warning, TEXT("dir.size()!=range.size()"));
  // Yawは上から見て反時計回りに定義されることに注意
  Yaws.Reserve(Yaws.Num() + dirs.Num());
  Ranges.Reserve(Ranges.Num() + ranges.Num());
  Intensities.Reserve(Intensities.Num() + intensities.Num());
  for (int i = 0, ec = ranges.Num(); i != ec; ++i) {
    bool inRange = dirs[i].Yaw - StartHAngle >= 0 && dirs[i].Yaw - EndHAngle <= 0;
      if ((InRange == true && inRange == false) || (inRange == true && LastYaw > dirs[i].Yaw)) {
        // 1スキャン終了
        //UE_LOG(LogTemp, Warning, TEXT(" pushscan Yaws.Num:%d Ranges.Num:%d Yaw:%f InRange:%d"), Yaws.Num(), Ranges.Num(), dirs[i].Yaw, inRange);
        sendPacket();
        Yaws.Empty();
        Ranges.Empty();
        Intensities.Empty();
      }
    if (inRange) {
      Ranges.Emplace(ranges[i]);
      Yaws.Emplace(-dirs[i].Yaw);   // 上から見て時計回りが+となるように向きを反転
      Intensities.Emplace(intensities[i]);
    }
    InRange = inRange;
    LastYaw = dirs[i].Yaw;
  }
}

// UDP Comm Protocol
void UScannerIOProtocolUDP::BeginPlay()
{
  Super::BeginPlay();
  RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

  auto sockName = FString::Printf(TEXT("ScanSocket-%s"), *GetOwner()->GetName());

  // store broadcast ip address
  bool bIsValid;
  RemoteAddr->SetIp(TEXT("255.255.255.255"), bIsValid);
  RemoteAddr->GetIp(BroadcastIP);
  
  setRemoteIP(RemoteIP);
  RemoteAddr->SetPort(RemotePort);
  Socket = FUdpSocketBuilder(sockName)
    .AsReusable()
    .WithSendBufferSize(8192*8)  // default x8
    .WithBroadcast();

}
bool UScannerIOProtocolUDP::setRemoteIP(const FString &remoteIP)
{
  if(ForceBroadcast){
    RemoteIP = FString("255.255.255.255");
  }
  else {
    RemoteIP = remoteIP;
  }
  bool bIsValid;
  RemoteAddr->SetIp(*RemoteIP, bIsValid);

  uint32 ipaddr;
  RemoteAddr->GetIp(ipaddr);
  RemoteIsBroadcast = (ipaddr == BroadcastIP);

  return bIsValid;
}

void UScannerIOProtocolUDP::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
  Socket->Close();
  delete(Socket);
  Socket = nullptr;
}

void UScannerIOProtocolUDP::sendPacket()
{
  if (!ensure(Scanner != nullptr)) return;

  // pack format 
  //   npts,ptsps,dir0[deg],range0[m],dir1,range1,....
  FBufferArchive pack;
  short count = Ranges.Num();
  pack << count;
  //short ptsps = Rpm / 60. * 360 / StepHAngle;
  short ptsps = 360. / Scanner->GetStepHAngle() / Scanner->GetFrameInterval();
  pack << ptsps;
  for (int i = 0, ec = count; i != ec; ++i) {
    pack << Ranges[i];
    pack << Yaws[i];
  }
  if (Socket != nullptr) {
    // check if broadcasting is allowed or not.
    if (CVarBroadcast.GetValueOnGameThread() == 0 && RemoteIsBroadcast) {
      return;
    }

    int32 bytesSent;
    Socket->SendTo(pack.GetData(), pack.TotalSize(), bytesSent, *RemoteAddr);
    //UE_LOG(LogTemp, Warning, TEXT("SendUDP:%d  Count:%d"), bytesSent, count);
  }
}

void UScannerIOProtocolJsonComm::BeginPlay()
{
  Super::BeginPlay();
  Comm.setup(GetOwner()->GetFName());
}

void UScannerIOProtocolJsonComm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
  Comm.tearDown();
}

void UScannerIOProtocolJsonComm::sendPacket()
{
  FScan2D Scan;
  Scan.Ranges=Ranges;
  Scan.Angles=Yaws;
  FNamedMessage *Msg(new FNamedMessage);
  Msg->Name = GetOwner()->GetName();
  FJsonObjectConverter::UStructToJsonObjectString(Scan, Msg->Message);  
  Comm.Send(Msg);
}

// ------------------------------------------------------------ Velodyne Protocol

void UScannerIOProtocolVelodyne::BeginPlay()
{
  Super::BeginPlay();
  PreparePacket();
}

void UScannerIOProtocolVelodyne::sendPacket()
{
  if (Socket != nullptr) {
    // check if broadcasting is allowed or not.
    if (CVarBroadcast.GetValueOnGameThread() == 0 && RemoteIsBroadcast) {
      return;
    }

    int32 bytesSent;
    if (Socket->SendTo(Pack.GetData(), Pack.TotalSize(), bytesSent, *RemoteAddr) == false) {
      UE_LOG(LogTemp, Warning, TEXT("SendUDP: bytes sent=%d  size=%d"), bytesSent,Pack.TotalSize());
    }
  }
  PreparePacket();
}

void UScannerIOProtocolVelodyne::PreparePacket()
{
  Pack.Empty();
  Pack.Seek(0);
  NDataBlocks = 0;
}

void UScannerIOProtocolVelodyne::pushScan(TArray<float> &ranges, TArray<float> &intensities, TArray<FRotator> &dirs, TArray<float> &timestamps)
{
  size_t pos = 0;
  ensure(ranges.Num() == intensities.Num());
  //uint8_t  intensity = 200;
  while (ranges.Num()-pos >= 16) {
    if (Rangedata.Num() == 0) {
      float yaw = dirs[pos].Yaw;
      if (yaw < 0)yaw += 360;
      Azimuth = yaw * 100;
      if(NDataBlocks==0)
        Timestamp = timestamps[pos];
    }
    for (int idx = 0; idx < 16; ++idx) {
      Rangedata.Add(ranges[pos + idx]/UnitRange);
      Intensities.Add(intensities[pos + idx] * IntensityDRange);
    }
    pos += 16;

    if (Rangedata.Num() <= 16)continue;

    // Datablock完成
    unsigned short blockType = 0xeeff;
    Pack << blockType;
    Pack << Azimuth;

    for (size_t p = 0; p < 32; ++p) {
      Pack << Rangedata[p];
      Pack << Intensities[p];
    }
    ++NDataBlocks;
    Rangedata.Empty();
    Intensities.Empty();

    // Packet完成
    if (NDataBlocks >= 12) {
      int32_t currentTime = Timestamp * 1000000; //GetWorld()->GetTimeSeconds() * 1000000;
      uint8_t mode = 0x37;  // strongest
      uint8_t model = ModelID;
      Pack << currentTime;
      Pack << mode;
      Pack << model;
      sendPacket();
    }
    //intensity++; // 同一フレームで処理したパケットをintensityを流用してマークする
  }
}
#if 0
void UScannerIOProtocolVelodyne::pushRange(TArray<uint16_t> &rangedata, TArray<float> & ranges, size_t pos)
{
  rangedata.Add(ranges[pos +  0] * 1 / 2);  // mm->2mm
  rangedata.Add(ranges[pos +  8] * 1 / 2);
  rangedata.Add(ranges[pos +  1] * 1 / 2);
  rangedata.Add(ranges[pos +  9] * 1 / 2);
  rangedata.Add(ranges[pos +  2] * 1 / 2);
  rangedata.Add(ranges[pos + 10] * 1 / 2);
  rangedata.Add(ranges[pos +  3] * 1 / 2);
  rangedata.Add(ranges[pos + 11] * 1 / 2);
  rangedata.Add(ranges[pos +  4] * 1 / 2);
  rangedata.Add(ranges[pos + 12] * 1 / 2);
  rangedata.Add(ranges[pos +  5] * 1 / 2);
  rangedata.Add(ranges[pos + 13] * 1 / 2);
  rangedata.Add(ranges[pos +  6] * 1 / 2);
  rangedata.Add(ranges[pos + 14] * 1 / 2);
  rangedata.Add(ranges[pos +  7] * 1 / 2);
  rangedata.Add(ranges[pos + 15] * 1 / 2);
}
#endif