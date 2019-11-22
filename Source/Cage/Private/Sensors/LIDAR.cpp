// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "LIDAR.h"
#include <Engine/Public/DrawDebugHelpers.h>
#include <Core/Public/Math/UnrealMathUtility.h>
#include <Core/Public/Async/ParallelFor.h>
DECLARE_STATS_GROUP(TEXT("CageSensors"), STATGROUP_CageSensors, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ALidar::Tick"), STAT_LidarTick, STATGROUP_CageSensors);
DECLARE_CYCLE_STAT(TEXT("ALidar::Trace"), STAT_LidarTrace, STATGROUP_CageSensors);
DECLARE_CYCLE_STAT(TEXT("ALidar::Send"), STAT_LidarSend, STATGROUP_CageSensors);

static TAutoConsoleVariable<int32> CVarVisualize(
  TEXT("cage.Lidar.Visualize"),
  5,
  TEXT("Visualize lidar reflection points.\n")
  TEXT(" 0: No points\n")
  TEXT(" 1: Draw all points.\n")
  TEXT(" N: Draw every N points.\n"),
  ECVF_RenderThreadSafe
);

void ALidar::BeginPlay()
{
  Super::BeginPlay();
  LastRotation = GetStaticMeshComponent()->GetSocketRotation(FName("Laser"));
  LastLocation = GetStaticMeshComponent()->GetSocketLocation(FName("Laser"));
  LastMeasureTime = GetWorld()->GetTimeSeconds();
  if (Scanner != nullptr) {
    Scanner->init(LastMeasureTime, this);
  }

#if 0
  // ad-hoc init code
  if (Scanner == nullptr) {
    Scanner = NewObject<UScanStrategy>(this);
    Scanner->init(LastMeasureTime);
    //Scanner->StepHAngle = StepHAngle;
    UE_LOG(LogTemp, Warning, TEXT("Scanner Created"));
  }
  if (IOProtocol == nullptr) {
    IOProtocol = NewObject<UScannerIOProtocol>(this);
    if (IOProtocol != nullptr) {
      IOProtocol->init(GetName());
      IOProtocol->EndHAngle = EndHAngle;
      IOProtocol->StartHAngle = StartHAngle;
      IOProtocol->Rpm = Rpm;
      IOProtocol->StepHAngle = StepHAngle;
      UE_LOG(LogTemp, Warning, TEXT("IOProtocol Created"));
    }
  }
  Yaw = StartHAngle;
#endif
  //FRotator spinner = FRotator(pitch, Yaw, 0);
}

void ALidar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

}

ALidar::ALidar():Rgen(Rdev())
{
  PrimaryActorTick.bCanEverTick = true;
}

void ALidar::Init()
{
  IOProtocol = FindComponentByClass<UScannerIOProtocol>();
  Scanner = FindComponentByClass<UScanStrategy>();
  if (IOProtocol == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("%s: IOProtocol component is missing"),*GetName());
  }
  if (Scanner == nullptr) {
    UE_LOG(LogTemp, Error, TEXT("%s: ScanStrategy component is missing"),*GetName());
  }
}

void ALidar::OnConstruction(const FTransform& Transform)
{
  Super::OnConstruction(Transform);
  Init();
}


void ALidar::SetPeerAddress(const FString &address)
{
  IOProtocol->setRemoteIP(address);
}

void ALidar::Tick(float dt)
{
  Super::Tick(dt);
  SCOPE_CYCLE_COUNTER(STAT_LidarTick);
  int hit = 0, miss = 0;
  float rangeSum = 0;
  float pitch = 0.;
  // 
  float currentTime = GetWorld()->GetTimeSeconds();
  float deltaTime = currentTime - LastMeasureTime;


  if (Scanner == nullptr)return;

  float dbgLifetime = Scanner->GetFrameInterval();
  FRotator currentRotation = GetStaticMeshComponent()->GetSocketRotation(FName("Laser"));
  FVector currentLocation = GetStaticMeshComponent()->GetSocketLocation(FName("Laser"));

  FQuat lastPose = LastRotation.Quaternion();
  FQuat curPose = currentRotation.Quaternion();
  { // draw green line indicating scanner orientation and min/max ranges
    DrawDebugLine(GetWorld(),
      currentLocation + currentRotation.RotateVector(FVector(1, 0, 0))*MinRange,
      currentLocation + currentRotation.RotateVector(FVector(1, 0, 0))*MaxRange,
      FColor(0, 250, 0), false, 0.f, 0.f, 0.5f);
  }

  // 計測方位セットを計算
  UScanStrategy::VecType scanVectors;
  float endTime = Scanner->GetVectors(scanVectors, currentTime);
  float currentMeasureDuration = endTime - LastMeasureTime;
  // endTime相当のpos, poseをslerpで計算, 次回のlastLocation, lastPoseとする
  auto endFactor = (endTime - LastMeasureTime) / (currentTime - LastMeasureTime);
  auto nextPose = FQuat::Slerp(lastPose, curPose, endFactor);
  auto nextPos = LastLocation * (1.f - endFactor) + currentLocation * endFactor;

  TArray<FRotator> scannedVectors;
  TArray<float> scanData;
  TArray<float> timeStamps;
  TArray<FVector> hitLocations;
  TArray<float> intensities;
  scanData.SetNumUninitialized(scanVectors.Num());
  timeStamps.SetNumUninitialized(scanVectors.Num());
  scannedVectors.SetNumUninitialized(scanVectors.Num());
  hitLocations.SetNumUninitialized(scanVectors.Num());
  intensities.SetNumUninitialized(scanVectors.Num());
  std::normal_distribution<float> rdist(0, NoiseDistribution);
  // 計測方位セットに従ってトレース
  {
    SCOPE_CYCLE_COUNTER(STAT_LidarTrace);
    ParallelFor(scanVectors.Num(), [&](int idx) {
      const auto &vec = scanVectors[idx];
      float factor = vec.Get<1>();
      scannedVectors[idx] = vec.Get < 0>();
      timeStamps[idx] = LastMeasureTime + factor * currentMeasureDuration;
      auto fv = FQuat::Slerp(lastPose, curPose, factor)
        .RotateVector(vec.Get<0>().RotateVector(FVector(1, 0, 0)));
      auto pos = LastLocation * (1.f - factor) + currentLocation * factor;
      auto traceStart = pos + fv * BodyRadius; //MinRange;
      FVector traceEnd = pos + fv * MaxRange;
      FHitResult resHit;
      FCollisionResponseParams resParams;
      FCollisionQueryParams params;
      params.bTraceComplex = true;
      params.bReturnPhysicalMaterial = true;
      float range = 0;
      float intensity = 0;
      bool res = GetWorld()->LineTraceSingleByChannel(resHit, traceStart, traceEnd, ECC_Visibility, params, resParams);
      if (res && resHit.Distance > MinRange - BodyRadius) {
        hitLocations[idx] = resHit.Location;
        range = (resHit.Distance + BodyRadius + rdist(Rgen)) * 10; // cm -> mm;
        // 正対したら上がる
        // 距離が離れると下がる
        // 10%が下限
        intensity = FMath::Clamp<float>(FVector::DotProduct(resHit.ImpactNormal, -fv)*0.5 +  (0.1 + 0.9 / range), 0., 1.);
        
#if 0   // ここで描画する場合にはParallelForをsingle threadにする必要がある
        if (++hit%5==1)
        {
          DrawDebugPoint(GetWorld(), hitLocations[idx], 5., FColor(250 * intensity, 0, 0), false, 0.5*dbgLifetime);
          DrawDebugLine(GetWorld(), resHit.Location, resHit.Location + resHit.ImpactNormal * 30, FColor(0, 250, 0), false, 0.5*dbgLifetime, 0.f, 0.2f);
          DrawDebugLine(GetWorld(), resHit.Location, resHit.Location - resHit.Normal * 30, FColor(250, 250, 0), false, 0.5*dbgLifetime, 0.f, 0.2f);
        }
#endif
      }
      scanData[idx] = range;
      intensities[idx] = intensity;
      }/*, true /*single thread*/); // ParallelFor
#if 1
    auto vis = CVarVisualize.GetValueOnGameThread();
    if (vis) {
      for (int idx = 0; idx < scanData.Num(); ++idx) {
        if (scanData[idx] != 0 && idx % vis == 0) {
          DrawDebugPoint(GetWorld(), hitLocations[idx], 5., FColor(250*intensities[idx], 0, 0), false, 0.5*dbgLifetime);
        }
      }
    }
#endif
  }
#if 0
  UE_LOG(LogTemp, Warning, TEXT("dt: %f ToScan: %d lines, Hit: %d lines YawRange: %f - %f "),
    dt, scanVectors.Num(),hit,
    scanVectors[0].Get<0>().Yaw,
    scanVectors[scanVectors.Num()-1].Get<0>().Yaw);

  UE_LOG(LogTemp, Warning, TEXT("Got %d linetraces, rangeSum=%f dbgLifetime=%f"),
    scannedVectors.Num(),
    rangeSum,
    dbgLifetime);
#endif
  // 結果を出力器に渡す
  {
    SCOPE_CYCLE_COUNTER(STAT_LidarSend);
    if (IOProtocol)
      IOProtocol->pushScan(scanData, intensities, scannedVectors, timeStamps);
    LastRotation = nextPose.Rotator();
    LastLocation = nextPos;
    LastMeasureTime = endTime;
  }
}
