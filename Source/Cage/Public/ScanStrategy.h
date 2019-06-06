// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
// Scan Strategy Interface
#include <Runtime/Core/Public/Containers/Array.h>
#include <Components/ActorComponent.h>
#include "ScanStrategy.generated.h"

class ALidar;

UCLASS(BlueprintType, ClassGroup="Sensors", Category="Lidar")
class UScanStrategy:public UActorComponent {
  GENERATED_BODY()
protected:
  float LastTime = 0;
public:
  // no functions callable from blueprint

  using VecType = TArray<TTuple<FRotator, float> >; //direction, interpolation factor
  void init(float time, ALidar* lidar) {
    LastTime = time;
    init(lidar);
  }
  virtual void init(ALidar *lidar) {}
  virtual float GetVectors(VecType &vec, float currentTime) { return 0; };
  virtual float GetFrameInterval() { return 0.1; };  // 1周期の時間を返す [s]
  virtual float GetStepHAngle() { return 0.1; }      // 水平解像度 [deg]
};

UCLASS(BlueprintType,Blueprintable, ClassGroup="Sensors", Category="Lidar")
class UScan2DStrategy : public UScanStrategy{
  GENERATED_BODY()
  float Yaw = 0;
public:
  virtual void init(ALidar *lidar);
  float GetVectors(VecType &vec, float currentTime) override;
  float GetFrameInterval() override { return 60.f / Rpm; }
  float GetStepHAngle() override { return StepHAngle; }

//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float StartHAngle;  // scan start angle [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float EndHAngle;     // scan end angle [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float StepHAngle;    // horizontal resolution [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float Rpm;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup = "Sensors", Category = "Lidar")
class UVLPScanStrategy : public UScanStrategy {
  GENERATED_BODY()
    float Yaw = 0;
public:
  virtual void init(ALidar *lidar);
  float GetVectors(VecType &vec, float currentTime) override;
  float GetFrameInterval() override { return 60.f / Rpm; }
  float GetStepHAngle() override { return StepHAngle; }

//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float StartHAngle;  // scan start angle [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float EndHAngle;     // scan end angle [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float StepHAngle;    // horizontal resolution [deg]
//  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float Rpm;
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
      TArray<float> VAngles = { -15,1, -13,3, -11,5, -9,7, -7,9, -5,11, -3,13, -1,15 }; // Channel allocation for VLP-16
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
      TArray<float> HOffsets;
};
