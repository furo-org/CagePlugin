// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/



#include "OmniMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Tests/AutomationTestSettings.h"

void UOmniMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    SetMovementMode(EMovementMode::MOVE_Walking);
    Mt=std::mt19937(Rd());
    Dist=std::normal_distribution<>(0.,1.);
}

void UOmniMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
    float clock=GetWorld()->GetTimeSeconds();
#if 0
    float cycle1=sin(CyclicDisturbHz*clock*PI*2);
    float cycle2=sin(CyclicDisturbHz*clock*PI*2*2);
    float cycle3=sin(CyclicDisturbHz*clock*PI*2*3);
    float cycleComp= CyclicDisturbAmp1*cycle1 + CyclicDisturbAmp2*cycle2 + CyclicDisturbAmp3*cycle3;    
    auto cyclicDisturb = FRotator(cycleComp, cycleComp, cycleComp);
#endif
    // rotation
    auto rot=GetOwner()->GetActorRotation();
    auto yawcmd= FRotator(0,ReferenceAngVel * DeltaTime,0);
    auto upright = FRotator(-rot.Pitch*UprightCoeff, 0, -rot.Roll*UprightCoeff);
    auto noise = FRotator(
    Dist(Mt)*PoseDisturbPitch*DeltaTime,
    Dist(Mt)*PoseDisturbYaw*DeltaTime,
    Dist(Mt)*PoseDisturbRoll*DeltaTime);
        
    auto combined = yawcmd + upright  + noise /*+ cyclicDisturb*/;
    GetOwner()->AddActorLocalRotation(combined);
#if 0
    static int seq=0;
    seq+=1;
    if(seq%50==0)
        UE_LOG(LogTemp, Warning, TEXT("rotvel: %s"),*combined.ToString());
#endif
    // velocity
    auto forward= GetOwner()->GetActorForwardVector()*ReferenceVel.X/MaxWalkSpeed;
    auto right = GetOwner()->GetActorRightVector()*ReferenceVel.Y/MaxWalkSpeed;
    const FVector driftVector(Dist(Mt),Dist(Mt),Dist(Mt));
    DriftState=FMath::LerpStable(DriftState, driftVector, DriftingCoeff);
    DriftState.Normalize();
    auto drift= DriftState * DriftMaxVel/MaxWalkSpeed * FVector(1.,1.,0);
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
    return TSharedPtr<FJsonObject>();
}

void UOmniMovementComponent::SetVW(FVector2D v, float w)
{
    ReferenceVel=v;
    ReferenceAngVel=w;
}
