// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Comm/Comm.h"
#include "TickUtils.h"
#include "ActorCommMgr.generated.h"


UINTERFACE()
class UActorCommClient : public UInterface {
  GENERATED_BODY()
public:
};

class IActorCommClient
{
  GENERATED_BODY()
public:
	virtual bool GetMetadata(FString &MetaOut)=0;
	virtual void CommRecv(const float DelstaTime, const TArray<TSharedPtr<FJsonObject>> &RcvJson)=0;
	virtual FString CommSend(const float DelstaTime)=0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAGE_API UActorCommMgr : public UActorComponent, public IPrePhysicsTickable, public IPostPhysicsTickable
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActorCommMgr();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void InitializeMetadata();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PrePhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostPhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(BlueprintReadOnly, Category = "SimVehicle")
	FString RemoteAddress="255.255.255.255";

protected:
	CommEndpointIO<FSimpleMessage> Comm;
	TArray<IActorCommClient*> CommClients;
	FString ActorMetadata;
};
