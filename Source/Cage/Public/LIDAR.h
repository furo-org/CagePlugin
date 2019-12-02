// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "ScannerIOProtocol.h"
#include "ScanStrategy.h"
#include <random>
#include "LIDAR.generated.h"
/**
 * 
 */

USTRUCT()
struct CAGE_API FIntensityParam {
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
    float Metaric;
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
    float Roughness;
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
    float Albedo;
  UPROPERTY(EditAnywhere, meta = (ClampMin = 0, ClampMax = 1))
    float Retroreflection;
};

USTRUCT()
struct CAGE_API FIntensityParamSetting: public FIntensityParam {
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
    TEnumAsByte<EPhysicalSurface> SurfaceType;
};

UCLASS(config=Project, defaultconfig)
class CAGE_API UIntensityResponseParams :public UObject {
  GENERATED_UCLASS_BODY()
  UPROPERTY(EditAnywhere, config, Category = IntensityResponse)
    FIntensityParam DefaultResponse;
  UPROPERTY(EditAnywhere, config, Category = IntensityResponse)
    TArray<FIntensityParamSetting> PerSurfaceTypeResponse;
};


UCLASS(ClassGroup = "Sensors", BluePrintable)
class CAGE_API ALidar : public AStaticMeshActor
{
	GENERATED_BODY()
  /* ---- */
public:
  //virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
  virtual void Tick(float deltaTime);
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  ALidar();

  virtual void Init();

  void UpdateIntensityParams();
  virtual void OnConstruction(const FTransform& Transform) override;

  UFUNCTION(BlueprintCallable, Category = "Setup")
    void SetPeerAddress(const FString &address);

  UPROPERTY(EditDefaultsOnly, Category = "Lidar")
    float MaxRange = 3000;     // range in [cm]
  UPROPERTY(EditDefaultsOnly, Category = "Lidar")
    float MinRange = 50;       // effective minimum range in [cm]
  UPROPERTY(EditDefaultsOnly, Category = "Lidar")
    float BodyRadius = 5;      // trace starting range in [cm]

  UPROPERTY(EditDefaultsOnly)
    float StartHAngle = -180.;  // scan start angle [deg]
  UPROPERTY(EditDefaultsOnly)
    float EndHAngle = 180.;     // scan end angle [deg]
  UPROPERTY(EditDefaultsOnly)
    float StepHAngle = 0.5;    // horizontal resolution [deg]
  UPROPERTY(EditDefaultsOnly)
    float Rpm = 1200;
  UPROPERTY(EditDefaultsOnly)
    float NoiseDistribution = 2; // Noise distribution add to distance response [cm]

  UPROPERTY(EditAnywhere)
    TArray<FIntensityParam> IntensityResponseParams;
  UPROPERTY(EditAnywhere)
    float IntensityScalingFactor = 1.;
protected:

  UPROPERTY(VisibleAnywhere,Category="Lidar")
  UScannerIOProtocol *IOProtocol=nullptr;
  UPROPERTY(VisibleAnywhere,Category="Lidar")
  UScanStrategy *Scanner=nullptr;
  FVector LastLocation;
  FRotator LastRotation;
  float Yaw;
  float LastMeasureTime;
  std::random_device Rdev;
  std::mt19937 Rgen;
};
