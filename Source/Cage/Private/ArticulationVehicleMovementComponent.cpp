// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ArticulationVehicleMovementComponent.h"
#include "ArticulationLinkComponent.h"
#include "Engine/Classes/GameFramework/Actor.h"
#include "Engine/Public/TimerManager.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"
#include "EngineUtils.h"
#include "CommActor.h"
#include<sstream>
#include<string>
#include "Comm/Types.h"

void FWheelController::setVelocityTargetRpm(float rpm)
{
    if (!ensure(Wheel))return;
    Wheel->Joint.SetVelocityTarget(FVector(rpm / 60., 0, 0));
}

float FWheelController::getCurrentRpm()
{
    if (!ensure(Wheel))return 0;
    return Wheel->Link.GetAngularVelocity()[0] * 60.;
}

// ------------------------------------------------------------

void UArticulationVehicleMovementComponent::setVW(float v, float w)
{
    if (!IsReady)return;
    //UE_LOG(LogTemp, Warning, TEXT("SetVW(%f,%f) TreadWidth:%f PerimeterR: %f"), v, w, TreadWidth, WheelR.Perimeter);
    RefVel = v;
    RefOmega = w;
    w *= PI / 180.;
    RefRpmRight = (RefVel + w * (TreadWidth)) / WheelR.Perimeter * 60. * -RotationDirection;
    RefRpmLeft = (RefVel - w * (TreadWidth)) / WheelL.Perimeter * 60. * RotationDirection;
}

void UArticulationVehicleMovementComponent::setRPM(float l, float r)
{
    if (!IsReady)return;
    RefRpmRight = r / WheelReductionRatio * RotationDirection;
    RefRpmLeft = l / WheelReductionRatio * RotationDirection;
}

void UArticulationVehicleMovementComponent::FixupReferences()
{
    if (WheelR.Wheel == nullptr && WheelR.TireName != NAME_None)
    {
        WheelR.Wheel = FindNamedArticulationLinkComponent(WheelR.TireName);
        if (WheelR.Wheel && WheelR.Wheel->Joint.IsVelocityDriveEnabled() == false)
        {
            UE_LOG(LogTemp, Error, TEXT("Right tire is not compatible with this component"));
            WheelR.Wheel = nullptr;
        }
    }
    if (WheelL.Wheel == nullptr && WheelL.TireName != NAME_None)
    {
        WheelL.Wheel = FindNamedArticulationLinkComponent(WheelL.TireName);
        if (WheelL.Wheel && WheelL.Wheel->Joint.IsVelocityDriveEnabled() == false)
        {
            UE_LOG(LogTemp, Error, TEXT("Left tire is not compatible with this component"));
            WheelL.Wheel = nullptr;
        }
    }

    if (WheelR.Wheel && WheelL.Wheel && WheelR.Wheel->IsStarted && WheelL.Wheel->IsStarted)
    {
        auto posL = WheelL.Wheel->GetComponentLocation();
        auto posR = WheelR.Wheel->GetComponentLocation();
        auto width = posL - posR;
        TreadWidth = width.Size();
        WheelL.Perimeter = WheelL.Wheel->Link.GetRadiusIfSphere() * 2 * PI;
        WheelR.Perimeter = WheelR.Wheel->Link.GetRadiusIfSphere() * 2 * PI;
        ensure(WheelR.Perimeter != 0. && WheelL.Perimeter != 0.);
        UE_LOG(LogTemp, Verbose, TEXT("[%s] FixupReferences R:N=%s Perimeter=%f L:N=%s Perimeter=%f"),
               *(this->GetName()), *(WheelR.Wheel->GetName()), WheelR.Perimeter, *(WheelL.Wheel->GetName()),
               WheelL.Perimeter);
        IsReady = true;
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] Is not Ready"), *(this->GetName()));
        if (!WheelR.Wheel)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[%s] WheelR.Wheel==nullptr"), *(this->GetName()));
        }
        else if (!WheelL.Wheel)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[%s] WheelL.Wheel==nullptr"), *(this->GetName()));
        }
        else if (!WheelR.Wheel->IsStarted)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[%s] WheelR.Wheel->IsStarted!=true"), *(this->GetName()));
        }
        else if (!WheelL.Wheel->IsStarted)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[%s] WheelL.Wheel->IsStarted!=true"), *(this->GetName()));
        }
        GetWorld()->GetTimerManager().
                    SetTimerForNextTick(this, &UArticulationVehicleMovementComponent::FixupReferences);
    }
}

UArticulationLinkComponent* UArticulationVehicleMovementComponent::FindNamedArticulationLinkComponent(FName name)
{
    if (name == NAME_None) return nullptr;
    TArray<UArticulationLinkComponent*> articulationLinks;
    this->GetOwner()->GetComponents(articulationLinks, false);
    for (auto& link : articulationLinks)
    {
        //UE_LOG(LogTemp, Warning, TEXT("ArticulationLink : %s"), *link->GetName());
        if (link->GetFName() == name)
        {
            return link;
        }
    }
    return nullptr;
}

bool UArticulationVehicleMovementComponent::GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)
{
    if(!IsReady) return false;

    MetaOut->SetNumberField("TreadWidth",TreadWidth);
    MetaOut->SetNumberField("ReductionRatio",WheelReductionRatio);
    MetaOut->SetNumberField("WheelPerimeterL",WheelL.Perimeter);
    MetaOut->SetNumberField("WheelPerimeterR",WheelR.Perimeter);
    return true;
}

void UArticulationVehicleMovementComponent::CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>> &Json)
{
    // Process all requests. The newest request over write older ones.
    for(auto &Jo: Json)
    {
        FString Type = Jo->GetStringField("CmdType");
        if (Type == "VW")
        {
            float v = Jo->GetNumberField("V");
            float w = Jo->GetNumberField("W");
            setVW(v, w); // cm/s, deg/s
        }
        if (Type == "RPM")
        {
            float r = Jo->GetNumberField("R");
            float l = Jo->GetNumberField("L");;
            setRPM(l, r);
        }
    }

    if (!ensure(WheelL.Wheel != nullptr))return;
    if (!ensure(WheelR.Wheel != nullptr))return;

    WheelL.setVelocityTargetRpm(RefRpmLeft);
    WheelR.setVelocityTargetRpm(RefRpmRight);
    CurRpmLeft = WheelL.getCurrentRpm() * WheelReductionRatio * RotationDirection;
    CurRpmRight = WheelR.getCurrentRpm() * WheelReductionRatio * RotationDirection;
    //UE_LOG(LogTemp, Warning, TEXT("RefR: %f CurR: %f  RefL: %f CurL: %f"), RefRpmRight, CurRpmRight, RefRpmLeft, CurRpmLeft);
}

TSharedPtr<FJsonObject> UArticulationVehicleMovementComponent::CommSend(const float DeltaTime, UActorCommMgr *CommMgr)
{
    FVehicleStatus vs;
    vs.LeftRpm = CurRpmLeft;
    vs.RightRpm = CurRpmRight;
    auto jo=FJsonObjectConverter::UStructToJsonObject(vs);
    return jo;
}

void UArticulationVehicleMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    IsReady = false;
    RefVel = 0;
    RefOmega = 0;
    RefRpmLeft = 0;
    RefRpmRight = 0;
    FixupReferences();
}
