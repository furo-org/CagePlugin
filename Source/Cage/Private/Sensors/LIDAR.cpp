// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "LIDAR.h"
#include <Engine/Public/DrawDebugHelpers.h>
#include <Core/Public/Math/UnrealMathUtility.h>
#include <PhysicalMaterials/PhysicalMaterial.h>
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

UIntensityResponseParams::UIntensityResponseParams(const FObjectInitializer& ObjectInitializer)
  :Super(ObjectInitializer),
  DefaultResponse  { 0.1, 0.5, 0.7, 0 }
{}


void ALidar::BeginPlay()
{
  Super::BeginPlay();
  LastRotation = GetStaticMeshComponent()->GetSocketRotation(FName("Laser"));
  LastLocation = GetStaticMeshComponent()->GetSocketLocation(FName("Laser"));
  LastMeasureTime = GetWorld()->GetTimeSeconds();
  if (Scanner != nullptr) {
    Scanner->init(LastMeasureTime, this);
  }
  UpdateIntensityParams();
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
  return;
}

void ALidar::UpdateIntensityParams()
{
  const UEnum *enumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPhysicalSurface"), true);
  if (!enumPtr) {
    UE_LOG(LogTemp, Warning, TEXT("[%s] No EPhysicalSurface enum found"), *(this->GetName()));
    return;
  }
  // Fill all IntensityResponseParams with Default response
  const UIntensityResponseParams* intensityResponseSettings = GetDefault<UIntensityResponseParams>();
  IntensityResponseParams.SetNumUninitialized(SurfaceType_Max);
  for (int i = (int)SurfaceType_Default; i < (int)SurfaceType_Max; ++i) {
    IntensityResponseParams[i] = intensityResponseSettings->DefaultResponse;
  }
  // for each Per surface type response entries
  for (int i = 0; i < intensityResponseSettings->PerSurfaceTypeResponse.Num(); ++i)
  {
    const auto &entry = intensityResponseSettings->PerSurfaceTypeResponse[i];
    //  copy params into specified IntensityResponseParams
    IntensityResponseParams[entry.SurfaceType] = static_cast<FIntensityParam>(entry);
    UE_LOG(LogTemp, Warning, TEXT("[%s] %s : albedo=%f  metaric=%f  roughness=%f  retro=%f"), *(this->GetName()),
      *enumPtr->GetDisplayNameTextByValue(entry.SurfaceType).ToString(),
      IntensityResponseParams[entry.SurfaceType].Albedo,
      IntensityResponseParams[entry.SurfaceType].Metaric,
      IntensityResponseParams[entry.SurfaceType].Roughness,
      IntensityResponseParams[entry.SurfaceType].Retroreflection
    );
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
        int surface = static_cast<int>(resHit.PhysMaterial->SurfaceType);
        const FIntensityParam *intensityParam=&IntensityResponseParams[surface];
        // Cook-Torrance Model
        float albedo = intensityParam->Albedo;
        float roughness = intensityParam->Roughness;
        float metalic = intensityParam->Metaric;
        float retro = intensityParam->Retroreflection;
        float roughness_sq = roughness * roughness;
        float dotproduct = FVector::DotProduct(resHit.ImpactNormal, -fv);
        float F = metalic + (1 - metalic)*powf(1 - dotproduct, 5);
        // Simplified GGX (https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf)
        float D = roughness_sq / (PI*powf(dotproduct*dotproduct*(roughness_sq - 1) + 1, 2));
        float V = 1./4.*(dotproduct*sqrt(dotproduct*dotproduct*(1-roughness_sq)+roughness_sq));
        float specular = D * V * F;
        float diffuse = albedo / PI * (dotproduct*(1-retro)+retro)*(1 - F);
        intensity = (diffuse + specular) / ((range/1000.)*(1-retro)+retro)*IntensityScalingFactor;
        intensity = FMath::Clamp<float>(intensity, 0., 1.);
        if (intensity < IntensityCutoff)
          range = 0;

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
