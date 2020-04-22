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
DECLARE_CYCLE_STAT(TEXT("ALidar::Intensity"), STAT_LidarTraceIntensity, STATGROUP_CageSensors);
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
    UE_LOG(LogTemp, Warning, TEXT("[%s] %s : id=%d albedo=%f  specular=%f  roughness=%f  retro=%f"), *(this->GetName()),
      *enumPtr->GetDisplayNameTextByValue(entry.SurfaceType).ToString(),
	  entry.SurfaceType,
      IntensityResponseParams[entry.SurfaceType].Albedo,
      IntensityResponseParams[entry.SurfaceType].Specular,
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
		SCOPE_CYCLE_COUNTER(STAT_LidarTraceIntensity);
        int surface = static_cast<int>(resHit.PhysMaterial->SurfaceType);
        const FIntensityParam *intensityParam=&IntensityResponseParams[surface];
        // Micro facet reflection model
        float albedo = intensityParam->Albedo;
        float roughness = intensityParam->Roughness;
		float specular = intensityParam->Specular;
        float retro = intensityParam->Retroreflection;
        float roughness_sq = roughness * roughness;
		float roughness_4 = roughness_sq * roughness_sq;
		float VN = FVector::DotProduct(resHit.ImpactNormal, -fv); // = LN
		float VN_sq = VN * VN;
		//constexpr float VH = 1; // = LH
		float F_s = specular;   // +(1 - specular)*powf(1 - VH, 5);  =0
        // Simplified GGX
		float d = VN_sq * (roughness_4 - 1) + 1;
        float D = roughness_4 / (PI*d*d);
		// Height-Correlated Masking and Shadowing
		float Lambda = (-1 + sqrt(1 + roughness_4 * (1 / VN_sq - 1)))/2.; // Lambda_GGX
		float G = 1. / (1 + 2 * Lambda);
		// retro
		float F_r = retro;
		float f_specular = D * G * F_s / (4 * VN_sq);
		float d_retro = 1/*retro reflective*/ * (roughness_4 - 1) + 1;
		float D_retro= roughness_4 / (PI*d_retro*d_retro);
		float f_retro = D_retro * (1 - F_s)*F_r;
		float f_diffuse = albedo / PI *(1-F_s)*(1-F_r);
		float range_m = range / 1000.;
		intensity = (f_diffuse + f_specular + f_retro )*(1. / (range_m*range_m))*IntensityScalingFactor;
        intensity = FMath::Clamp<float>(intensity, 0., 1.);
        if (intensity < IntensityCutoff)
          range = 0;
#if 0
			if (++hit % 50 == 1 && specular>0.9  /*surface ==2*/)
				UE_LOG(LogTemp, Warning, TEXT("#4 s:%d i:%f diffuse:%f specular:%f retro:%f range_m:%f F_s:%f F_r:%f D:%f G:%f roughness2:%f dp2:%f"),
					surface,intensity, f_diffuse, f_specular, f_retro, range_m, F_s, F_r, D, G,roughness_sq,VN_sq);
#endif
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
  if (currentTime < 2 && currentTime>1) {
	  const UEnum *enumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPhysicalSurface"), true);
	  if (!enumPtr) {
		  UE_LOG(LogTemp, Warning, TEXT("[%s] No EPhysicalSurface enum found"), *(this->GetName()));
	  }
	  else {
		  UE_LOG(LogTemp, Warning, TEXT("IntensityReponseParam Dump: Entrycount=%d"), IntensityResponseParams.Num());
		  for (int i = 0; i < IntensityResponseParams.Num(); ++i)
		  {
			  UE_LOG(LogTemp, Warning, TEXT("[%s] %s : id=%d albedo=%f  specular=%f  roughness=%f  retro=%f"), *(this->GetName()),
				  *enumPtr->GetDisplayNameTextByValue(i).ToString(),
				  i,
				  IntensityResponseParams[i].Albedo,
				  IntensityResponseParams[i].Specular,
				  IntensityResponseParams[i].Roughness,
				  IntensityResponseParams[i].Retroreflection
			  );
		  }
	  }
  }
#endif

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
