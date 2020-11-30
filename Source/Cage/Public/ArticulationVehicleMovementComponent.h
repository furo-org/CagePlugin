// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Comm/Comm.h"
#include "Comm/ActorCommMgr.h"
#include "GameFramework/MovementComponent.h"
#include "ArticulationVehicleMovementComponent.generated.h"

class UArticulationLinkComponent;
class UArticulationVehicleMovementComponent;

USTRUCT(BlueprintType)
struct FWheelController{
  GENERATED_BODY()
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Perimeter;
  UPROPERTY(EditDefaultsOnly)
  FName TireName;

  FWheelController(FName tireName) :Perimeter(KINDA_SMALL_NUMBER), TireName(tireName) {}
  FWheelController() :Perimeter(KINDA_SMALL_NUMBER), TireName(NAME_None) {}
  void setVelocityTargetRpm(float rpm);
  float getCurrentRpm();
private:
  UArticulationLinkComponent *Wheel=nullptr;
  friend class UArticulationVehicleMovementComponent;
};

/**
 * 
 */
UCLASS(meta = (BlueprintSpawnableComponent), ClassGroup = (Custom))
class CAGE_API UArticulationVehicleMovementComponent : public UMovementComponent, public IActorCommClient, public IActorCommMetaSender
{
  GENERATED_BODY()

public:
  UPROPERTY(EditDefaultsOnly, Category = "Actuators")
    FWheelController WheelR = {FName(TEXT("Wheel-R"))};
  UPROPERTY(EditDefaultsOnly, Category = "Actuators")
    FWheelController WheelL = {FName(TEXT("Wheel-L"))};
  UPROPERTY(EditDefaultsOnly, Category = "Actuators")
    float WheelReductionRatio = 15;
  // 1 or -1
  UPROPERTY(EditDefaultsOnly, Category = "Actuators")
    int RotationDirection = -1;
protected:
	UArticulationVehicleMovementComponent()=default;

  void FixupReferences();
  UArticulationLinkComponent *FindNamedArticulationLinkComponent(FName name);
  // IActorCommClient
  virtual bool GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut) override;
  virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>> &Json) override;
  TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr *CommMgr) override;

  bool IsReady;
  //FTickFunction SecondaryTick;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float RefVel = 0;
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float RefOmega = 0;    // Reference angular velocity [deg/s]
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float RefRpmLeft = 0;  // Reference left tire speed [rpm]
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float RefRpmRight = 0;  // Reference right tire speed [rpm]
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float CurRpmLeft = 0;   // current left motor speed [rpm]
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SimVehicle")
    float CurRpmRight = 0;   // current left motor speed [rpm]

  UPROPERTY(BlueprintReadOnly)
		float YawAngle;
  UPROPERTY(BlueprintReadOnly)
    FVector Position;
  UPROPERTY(BlueprintReadOnly)
    FQuat Pose;
  UPROPERTY(BlueprintReadOnly)
    float TreadWidth;

public:
  UFUNCTION(BluePrintCallable, Category = "Movement")
    void setVW(float v, float w); // [cm/s], [rad/s]
  UFUNCTION(BluePrintCallable, Category = "Movement")
    void setRPM(float l, float r); // set motor speed [rpm]. Tire speed=motor speed/wheelReductionRatio.

	virtual void BeginPlay() override;
};
