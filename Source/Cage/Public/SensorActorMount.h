// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "Comm/ActorCommMgr.h"
#include "SensorActorMount.generated.h"
/**
 *   Child Actor Component with communication capability with ActorCommMgr Component 
 */
UCLASS(ClassGroup=Utility, hidecategories=(Object,LOD,Physics,Lighting,TextureStreaming,Activation,"Components|Activation",Collision), meta=(BlueprintSpawnableComponent))
class CAGE_API USensorActorMount : public UChildActorComponent, public IActorCommListener, public IActorCommMetaSender, public IActorCommClient
{
	GENERATED_BODY()
	public:

	virtual void BeginPlay() override;

	virtual bool GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut) override;

	virtual void RemoteAddressChanged(const FString& Address) override;


	virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>>& RcvJson) override;
	virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr* CommMgr) override;
	
	protected:
	UActorCommMgr *ChildCommMgr=nullptr;
};
