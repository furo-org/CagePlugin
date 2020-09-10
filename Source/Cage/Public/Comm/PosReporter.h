// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once


#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Comm/Comm.h"
#include "ActorCommMgr.h"
#include "GeoReference.h"

#include "PosReporter.generated.h"

class AGeoReference;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UPosReporter : public UActorComponent, public IActorCommClient
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPosReporter()=default;
	virtual ~UPosReporter()=default;

public:

	virtual void BeginPlay() override;
	virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr *CommMgr) override;
protected:
	UPROPERTY(EditAnywhere)
	bool SendGeoLocation=false;  // Send Latitude and Longitude corresponding to x, y

	AGeoReference *GeoReference=nullptr;
};
