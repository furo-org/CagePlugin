// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TickUtils.h"
#include "ActorCommMgrAPI.h"
#include "ActorCommMgr.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAGE_API UActorCommMgr : public UActorComponent, public IPrePhysicsTickable, public IPostPhysicsTickable,
 public IActorCommClient, public IActorCommListener, public IActorCommMetaSender
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActorCommMgr();

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	void Initialize();
	bool CollectMetadata();

public:	
	// IPrePhysicsTickable
	virtual void PrePhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// IPostPhysicsTickable
	virtual void PostPhysicsTick(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// IActorCommMetaSender
	virtual bool GetMetadata(UActorCommMgr* CommMgr, TSharedRef<FJsonObject> MetaOut) override;
	// IActorCommListener
	virtual void RemoteAddressChanged(const FString& Address) override;

	// Remote IP Address which sends 'ActorCmd' to this actor
	UPROPERTY(BlueprintReadOnly, Category = "SimVehicle")
	FString RemoteAddress="255.255.255.255";

	// Name of Socket which defines Base transformation (ROS base_link). Transform of root component is used if None.
	UPROPERTY(EditAnywhere, Category="Setup")
	FName BaseSocket;
	
	FTransform GetBaseTransform();  // Get world to base transform
	
protected:
	class FImpl;
	FImpl *D = nullptr;
	USceneComponent *BaseComp=nullptr;
};

inline bool JsonObjectToFString(TSharedRef<FJsonObject> Json, FString &StringOut)
{
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> > JsonWriter =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&StringOut, 0);
	bool ret = FJsonSerializer::Serialize(Json, JsonWriter);
	JsonWriter->Close();
	return ret;
}
