// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "ActorCommMgr.h"
#include "CoreMinimal.h"

#include <random>

#include "GameFramework/CharacterMovementComponent.h"
#include "OmniMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class CAGE_API UOmniMovementComponent : public UCharacterMovementComponent, public IActorCommClient, public IActorCommMetaSender
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool GetMetadata(UActorCommMgr* CommMgr, TSharedRef<FJsonObject> MetaOut) override;
	virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>>& RcvJson) override;
	virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr* CommMgr) override;

	UFUNCTION(BluePrintCallable, Category = "Movement")
      void SetVW(FVector2D v, float w); // Velocity (Fwd, Right) [cm/s], Angular velocity (CW)[rad/s]

	UPROPERTY(BlueprintReadOnly)
	FVector2D ReferenceVel;
	UPROPERTY(BlueprintReadOnly)
	float ReferenceAngVel;
	UPROPERTY(BlueprintReadOnly)
	float CurrentAngVel;
	UPROPERTY(EditAnywhere)
	float MaxAngAccel=60;      // max anngular accel [deg/s^2]
	UPROPERTY(BlueprintReadOnly, Category=Disturbance)
	FVector DriftState;   // Current normalized drifting velocity vector. 
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float DriftingCoeff;  // 0...1; 0:=Keep current drifting velocity, 1:=Update drifting velocity with random values 
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float DriftMaxVel;    // Maximum velocity of random drifting velocity input [cm/s]
	/**
	 * @brief Hardness of upright enforcer. Upright pose is enforced bt applying a rotation every tick. This rotation is
	 *        multiplied with UprightCoeff before applying it to the actor.
	 *    (0: No Upright enforcing applied,  1: Up vector fully maintained every tick) 
	 */
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float UprightCoeff;
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float PoseDisturbRoll;
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float PoseDisturbPitch;
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float PoseDisturbYaw;
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float MaxRandomAngularVelocityBias;   // Maximum angular velocity of randomly initialized bias [deg/s] 
	UPROPERTY(EditAnywhere, Category=Disturbance)
	float MaxRandomAngularVelocityScale;  // Maximum angular velocity command error proportional to linear velocity
	UPROPERTY(EditAnywhere, Category=Disturbance)
	FVector2D MaxRandomVelocityBias;      // Maximum linear velocity of randomly initialized bias [cm/s]
	UPROPERTY(EditAnywhere, Category=Disturbance)
	FVector2D MaxRandomVelocityScale;     // Maximum linear velocity command error proportional to linear velocity
	
	protected:
	std::random_device Rd;
	std::mt19937 Mt;
	std::normal_distribution<> Dist;
	float RandomAngularVelBias;
	float RandomAngularVelScale;
	FVector2D RandomVelocityBias;
	FVector2D RandomVelocityScale;
};
