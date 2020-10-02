// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "ActorCommMgr.h"
#include "CoreMinimal.h"
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
	FVector DriftState;
	UPROPERTY(EditAnywhere)
	float DriftingCoeff;
	UPROPERTY(EditAnywhere)
	float DriftMaxVel;
};
