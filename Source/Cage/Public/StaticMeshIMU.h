// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Comm/ActorCommMgr.h"
#include "StaticMeshIMU.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
//class CAGE_API USimpleIMU : public USceneComponent, public IActorCommClient, public IActorCommMetaSender
class CAGE_API UStaticMeshIMU : public UStaticMeshComponent, public IActorCommClient, public IActorCommMetaSender
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStaticMeshIMU()=default;

	UPROPERTY(BlueprintReadOnly)
	float YawRate;
	UPROPERTY(BlueprintReadOnly)
	float RollRate;
	UPROPERTY(BlueprintReadOnly)
	float PitchRate;
	UPROPERTY(BlueprintReadOnly)
	float Vel;
	UPROPERTY(BlueprintReadOnly)
	FVector RateGyro;
	UPROPERTY(BlueprintReadOnly)
	FVector Accel;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// EMA coefficient
	UPROPERTY(EditAnywhere, Category = "IMU", meta = (UIMin = "0", UIMax = "1.0", ClampMin = "0", ClampMax = "1.0"))
	float AccelEMACoeff = 0.7;
	UPROPERTY(EditAnywhere, Category = "IMU", meta = (UIMin = "0", UIMax = "1.0", ClampMin = "0", ClampMax = "1.0"))
	float GyroEMACoeff = 0.9;
	FTransform PrevTransform;
	FVector PrevVelocity;

	// IActorCommClient
	virtual bool GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut) override;
	virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>> &Json) override;
	virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr *CommMgr) override;

};
