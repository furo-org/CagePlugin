// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "PosReporter.h"
#include "MessageEndpointBuilder.h"
#include "IMessageContext.h"
#include "AllowWindowsPlatformTypes.h"
#include "zmq_nt.hpp"
#include "HideWindowsPlatformTypes.h"
#include "Comm/Types.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"


// Sets default values for this component's properties
UPosReporter::UPosReporter()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // ...
}


// Called when the game starts
void UPosReporter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("BeginPlay "));
    Comm.setup(GetOwner()->GetFName());
}

void UPosReporter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Comm.tearDown();
}


// Called every frame
void UPosReporter::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    FVector loc = GetOwner()->GetActorLocation();
    FRotator rot = GetOwner()->GetActorRotation();

    FNamedMessage* msg(new FNamedMessage);
    FActorPosition rep;
    rep.Position = loc;
    rep.Yaw = rot.Yaw;
    FJsonObjectConverter::UStructToJsonObjectString(rep, msg->Message);
    msg->Name = GetOwner()->GetName();
    Comm.Send(msg);
    //UE_LOG(LogTemp, Warning, TEXT("PositionReporter: Tick "));
}
