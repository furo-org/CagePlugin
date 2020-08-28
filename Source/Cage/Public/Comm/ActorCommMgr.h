// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

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
	virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>> &RcvJson){};
	virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr *CommMgr){return TSharedPtr<FJsonObject>();};
};

UINTERFACE()
class UActorCommListener : public UInterface {
	GENERATED_BODY()
public:
};

class IActorCommListener
{
	GENERATED_BODY()
public:
	virtual void RemoteAddressChanged(const FString &Address)=0;
};

UINTERFACE()
class UActorCommMetaSender : public UInterface {
	GENERATED_BODY()
public:
};

class UActorCommMgr;

class IActorCommMetaSender
{
	GENERATED_BODY()
public:
	virtual bool GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)=0;
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
	virtual void PrePhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostPhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Remote IP Address which sends 'ActorCmd' to this actor
	UPROPERTY(BlueprintReadOnly, Category = "SimVehicle")
	FString RemoteAddress="255.255.255.255";

	// Name of Socket which defines Base transformation
	UPROPERTY(EditAnywhere, Category="Setup")
	FName BaseSocket;
	
	FTransform GetBaseTransform();  // Get world to base transform
	
protected:
	CommEndpointIO<FSimpleMessage> Comm;
	TArray<IActorCommClient*> CommClients;
	TArray<IActorCommListener*> CommListeners;
	FString ActorMetadata;
};

inline bool JsonObjectToFString(TSharedRef<FJsonObject> Json, FString &StringOut)
{
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> > JsonWriter =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&StringOut, 0);
	bool ret = FJsonSerializer::Serialize(Json, JsonWriter);
	JsonWriter->Close();
	return ret;
}
