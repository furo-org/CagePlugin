// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/



#include "OmniMovementComponent.h"
#include "Math/UnrealMathUtility.h"

void UOmniMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    SetMovementMode(EMovementMode::MOVE_Walking);
}

void UOmniMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
    // rotation
    GetOwner()->AddActorLocalRotation(FRotator(0,ReferenceAngVel*DeltaTime,0));
    // velocity
    auto rot=GetOwner()->GetActorRotation();
    auto forward= GetOwner()->GetActorForwardVector()*ReferenceVel.X/MaxWalkSpeed;
    auto right = GetOwner()->GetActorRightVector()*ReferenceVel.Y/MaxWalkSpeed;
    const FVector driftVector(FMath::FRand(),FMath::FRand(),FMath::FRand());
    DriftState=FMath::Lerp(DriftState, driftVector, DriftingCoeff);
    DriftState.Normalize();
    auto drift= DriftState * DriftMaxVel/MaxWalkSpeed * FVector(1,1,0);
    auto worldVel= forward + right + drift;
    AddInputVector(worldVel);
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
}

bool UOmniMovementComponent::GetMetadata(UActorCommMgr* CommMgr, TSharedRef<FJsonObject> MetaOut)
{
    return true;
}

void UOmniMovementComponent::CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>>& RcvJson)
{
    // Process all requests. The newest request over write older ones.
    for(auto &Jo: RcvJson)
    {
        FString Type = Jo->GetStringField("CmdType");
        if (Type == "VW")
        {
            double vx = 0, vy=0, w=0;
            Jo->TryGetNumberField("V",vx);
            Jo->TryGetNumberField("L",vy);
            Jo->TryGetNumberField("W",w);
            vy*=-1;
            w*=-1;
            SetVW(FVector2D(vx,vy), w); // cm/s, deg/s
        }
    }
}

TSharedPtr<FJsonObject> UOmniMovementComponent::CommSend(const float DeltaTime, UActorCommMgr* CommMgr)
{
    return MakeShared<FJsonObject>();
}

void UOmniMovementComponent::SetVW(FVector2D v, float w)
{
    ReferenceVel=v;
    ReferenceAngVel=w;
}
