// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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

UArticulationVehicleMovementComponent::UArticulationVehicleMovementComponent()
{
}

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

void UArticulationVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                                          FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UArticulationVehicleMovementComponent::UpdateStatus(float DeltaTime)
{
    //auto vehicleRoot = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
    auto* vehicleRoot = Cast<UArticulationLinkComponent>(GetOwner()->GetRootComponent());
    FTransform currentTransform = vehicleRoot->GetComponentTransform();
    FTransform deltaTransform = currentTransform.GetRelativeTransform(PrevTransform);

    auto deltaRotation = deltaTransform.GetRotation().Euler() / DeltaTime;
    RateGyro = GyroEMACoeff * deltaRotation + (1 - GyroEMACoeff) * RateGyro;
    float transformPitch = deltaRotation.Y;
    float transformRoll = deltaRotation.X;
    float transformYaw = deltaRotation.Z;

    PitchRate = transformPitch;
    RollRate = transformRoll;
    YawRate = transformYaw;

    YawAngle = currentTransform.GetRotation().Euler().Z;
    Pose = currentTransform.GetRotation();

    PrevTransform = currentTransform;
    auto velVec = GetOwner()->GetVelocity();
    Vel = FVector::DotProduct(velVec, GetOwner()->GetActorForwardVector());
    auto accelVec = (velVec - PrevVelocity) / DeltaTime;
    auto accel = currentTransform.GetRotation().UnrotateVector(accelVec + FVector(0, 0, 980));
    Accel = AccelEMACoeff * accel + (1.0 - AccelEMACoeff) * Accel;
    //UE_LOG(LogTemp, Warning, TEXT("[%s] dt: %f Vel: %f %f %f   Accel: %f %f %f" (EMA:%f), *(this->GetName()), DeltaTime, velVec.X, velVec.Y, velVec.Z, Accel.X, Accel.Y, Accel.Z, AccelEMACoeff);
    PrevVelocity = velVec;
#if 0
  FVector lin, ang;
  vehicleRoot->GetWorldVelocity(lin, ang);
  UE_LOG(LogTemp, Warning, TEXT("[%s] lin: %f %f %f   ang: %f %f %f"),*(this->GetName()),
    lin.X, lin.Y, lin.Z,  ang.X, ang.Y, ang.Z);
#endif
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

bool UArticulationVehicleMovementComponent::GetMetadata(FString& MetaOut)
{
    if(!IsReady) return false;

    MetaOut = FString::Printf(TEXT(
        "\"TreadWidth\":%f,\n"
        "\"ReductionRatio\":%f,\n"
        "\"WheelPerimeterL\":%f,\n"
        "\"WheelPerimeterR\":%f\n"),
                           TreadWidth,
                           WheelReductionRatio,
                           WheelL.Perimeter,
                           WheelR.Perimeter
    );
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

#if 0
  UE_LOG(LogTemp, Warning, TEXT("V[Ref:%f Cur:%f] W[Ref:%f Cur:%f] L[Ref:%f Cur:%s] R[Ref:%f Cur:%s]"), 
    RefVel, Vel, RefOmega, YawRate,
    RefRpmLeft,*(WheelL.Wheel->Link.GetAngularVelocity().ToString()),
    RefRpmRight, *(WheelR.Wheel->Link.GetAngularVelocity().ToString()));
  UE_LOG(LogTemp, Warning, TEXT("[%s] R: ref=%f cur=%f  L: ref=%f cur=%f"), *(this->GetName()),
    RefRpmRight, CurRpmRight,
    RefRpmLeft, CurRpmLeft);
#endif
}

FString UArticulationVehicleMovementComponent::CommSend(const float DeltaTime)
{
    UpdateStatus(DeltaTime);
    FVector loc = GetOwner()->GetActorLocation();
    FRotator rot = GetOwner()->GetActorRotation();

    FString jsonstr;
    FVehicleStatus vs;
    vs.Position = loc;
    vs.Pose = Pose;
    vs.AngVel = RateGyro; //FVector(RollRate, PitchRate, YawRate),
    vs.Accel = Accel;
    vs.Yaw = rot.Yaw;
    vs.LeftRpm = CurRpmLeft;
    vs.RightRpm = CurRpmRight;
    FJsonObjectConverter::UStructToJsonObjectString(vs, jsonstr);
#if 0
    auto jo=FJsonObjectConverter::UStructToJsonObject(vs);
    UE_LOG(LogTemp, Warning, TEXT("AVMC: reporting [%s]"), *msg->Message);
    for(auto &Elem: jo->Values)
    {
        UE_LOG(LogTemp,Warning,TEXT("Key : %s"),*Elem.Key);
    }
#endif
    return jsonstr;
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

void UArticulationVehicleMovementComponent::RegisterComm()
{

}

void UArticulationVehicleMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    Comm.tearDown();
}
