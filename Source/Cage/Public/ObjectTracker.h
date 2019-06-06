// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Comm/Comm.h"
#include "ObjectTracker.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAGE_API UObjectTracker : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UObjectTracker();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

  UPROPERTY(EditAnywhere, Category = "Setup")
    TArray<FName> TargetTags;
  UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float MaxRange=800;

protected:
  CommEndpointOut Comm;

};
