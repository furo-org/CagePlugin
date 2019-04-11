// Fill out your copyright notice in the Description page of Project Settings.
#include "LIDAR.h"
#include <Engine/Public/DrawDebugHelpers.h>
#include <Core/Public/Async/ParallelFor.h>
DECLARE_STATS_GROUP(TEXT("CageSensors"), STATGROUP_CageSensors, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ALidar::Tick"), STAT_LidarTick, STATGROUP_CageSensors);

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

  float dbgLifetime =Scanner->GetFrameInterval();
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
  scanData.SetNumUninitialized(scanVectors.Num());
  timeStamps.SetNumUninitialized(scanVectors.Num());
  scannedVectors.SetNumUninitialized(scanVectors.Num());
  hitLocations.SetNumUninitialized(scanVectors.Num());
  std::normal_distribution<float> rdist(0, NoiseDistribution);

  // 計測方位セットに従ってトレース
  ParallelFor(scanVectors.Num(), [&](int idx) {
  //for (int idx = 0; idx < scanVectors.Num(); ++idx) {
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
    float range = 0;
    bool res = GetWorld()->LineTraceSingleByChannel(resHit, traceStart, traceEnd, ECC_Visibility, params, resParams);
    if (res && resHit.Distance > MinRange-BodyRadius) {
      hitLocations[idx] = resHit.Location;
      range = (resHit.Distance + BodyRadius + rdist(Rgen)) * 10; // cm -> mm;
      //rangeSum += (resHit.Location - traceStart).Size();
      //if (++hit % 150 == 0) {
        //DrawDebugLine(GetWorld(), traceStart, resHit.Location, FColor(250, 0, 0), false, 0.5*dbgLifetime, 0.f, 0.5f);
      //}
      //DrawDebugLine(GetWorld(), resHit.Location, resHit.Location + resHit.ImpactNormal, FColor(250, 0, 250), false, 0.5*dbgLifetime);
    }
    scanData[idx] = range;
  //}  // for
  }); // ParallelFor
  for (int idx = 0; idx < scanData.Num(); ++idx) {
    if (scanData[idx] != 0) {
      DrawDebugPoint(GetWorld(), hitLocations[idx], 5., FColor(250, 0, 0), false, 0.5*dbgLifetime);
    }
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
  if (IOProtocol)
    IOProtocol->pushScan(scanData, scannedVectors, timeStamps);
  LastRotation = nextPose.Rotator();
  LastLocation = nextPos;
  LastMeasureTime = endTime;
}
