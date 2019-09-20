// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ArticulationVehicleMovementComponent.h"
#include "ArticulationLinkComponent.h"
#include "Engine/Classes/GameFramework/Actor.h"
#include "Engine/Public/TimerManager.h"
#include "EngineUtils.h"
#include "CommActor.h"
#include<sstream>
#include<string>
// cereal-UE4
#include "cerealUE4.hh"

void FWheelController::setVelocityTargetRpm(float rpm)
{
  if (!ensure(Wheel))return;
  Wheel->Joint.SetVelocityTarget(FVector(rpm/60., 0, 0));
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
	RefRpmLeft  = (RefVel - w * (TreadWidth)) / WheelL.Perimeter * 60. * RotationDirection;
}

void UArticulationVehicleMovementComponent::setRPM(float l, float r)
{
  if (!IsReady)return;
  RefRpmRight = r / WheelReductionRatio * RotationDirection;
  RefRpmLeft = l / WheelReductionRatio * RotationDirection;
}

void UArticulationVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  UpdateStatus(DeltaTime);

  if (!ensure(WheelL.Wheel != nullptr))return;
  if (!ensure(WheelR.Wheel != nullptr))return;

  CommRecv();

  WheelL.setVelocityTargetRpm(RefRpmLeft);
  WheelR.setVelocityTargetRpm(RefRpmRight);
  CurRpmLeft = WheelL.getCurrentRpm()*WheelReductionRatio * RotationDirection;
  CurRpmRight = WheelR.getCurrentRpm()*WheelReductionRatio * RotationDirection;
  //UE_LOG(LogTemp, Warning, TEXT("RefR: %f CurR: %f  RefL: %f CurL: %f"), RefRpmRight, CurRpmRight, RefRpmLeft, CurRpmLeft);

  CommSend();
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

void UArticulationVehicleMovementComponent::UpdateStatus(float DeltaTime)
{
  //auto vehicleRoot = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
  auto *vehicleRoot = Cast<UArticulationLinkComponent>(GetOwner()->GetRootComponent());
	FTransform currentTransform = vehicleRoot->GetComponentTransform();
	FTransform deltaTransform = currentTransform.GetRelativeTransform(PrevTransform);

	auto deltaRotation = deltaTransform.GetRotation().Euler()/DeltaTime;
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
  Accel = AccelEMACoeff * accel + (1.0 - AccelEMACoeff)*Accel;
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
  if (WheelR.Wheel == nullptr && WheelR.TireName != NAME_None) {
    WheelR.Wheel = FindNamedArticulationLinkComponent(WheelR.TireName);
    if (WheelR.Wheel && WheelR.Wheel->Joint.IsVelocityDriveEnabled() == false) {
      UE_LOG(LogTemp, Error, TEXT("Right tire is not compatible with this component"));
      WheelR.Wheel = nullptr;
    }
  }
  if (WheelL.Wheel == nullptr && WheelL.TireName != NAME_None) {
    WheelL.Wheel = FindNamedArticulationLinkComponent(WheelL.TireName);
    if (WheelL.Wheel && WheelL.Wheel->Joint.IsVelocityDriveEnabled() == false) {
      UE_LOG(LogTemp, Error, TEXT("Left tire is not compatible with this component"));
      WheelL.Wheel = nullptr;
    }
  }

  if (WheelR.Wheel && WheelL.Wheel && WheelR.Wheel->IsStarted && WheelL.Wheel->IsStarted){
    auto posL = WheelL.Wheel->GetComponentLocation();
    auto posR = WheelR.Wheel->GetComponentLocation();
    auto width = posL - posR;
    TreadWidth = width.Size();
    WheelL.Perimeter = WheelL.Wheel->Link.GetRadiusIfSphere() * 2 * PI;
    WheelR.Perimeter = WheelR.Wheel->Link.GetRadiusIfSphere() * 2 * PI;
    ensure(WheelR.Perimeter != 0. && WheelL.Perimeter != 0.);
    UE_LOG(LogTemp, Warning, TEXT("[%s] FixupReferences R:N=%s Perimeter=%f L:N=%s Perimeter=%f"),
      *(this->GetName()), *(WheelR.Wheel->GetName()), WheelR.Perimeter, *(WheelL.Wheel->GetName()), WheelL.Perimeter);
    IsReady = true;
    RegisterComm();
  }
  else {
    UE_LOG(LogTemp, Warning, TEXT("Is not Ready"));
    if (!WheelR.Wheel) {
      UE_LOG(LogTemp, Warning, TEXT("WheelR.Wheel==nullptr"));
    }
    else if (!WheelL.Wheel) {
      UE_LOG(LogTemp, Warning, TEXT("WheelL.Wheel==nullptr"));
    }
    else if (!WheelR.Wheel->IsStarted) {
      UE_LOG(LogTemp, Warning, TEXT("WheelR.Wheel->IsStarted!=true"));
    }
    else if (!WheelL.Wheel->IsStarted) {
      UE_LOG(LogTemp, Warning, TEXT("WheelL.Wheel->IsStarted!=true"));
    }
    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UArticulationVehicleMovementComponent::FixupReferences);
  }
}

UArticulationLinkComponent * UArticulationVehicleMovementComponent::FindNamedArticulationLinkComponent(FName name)
{
  if (name == NAME_None) return nullptr;
  TArray<UArticulationLinkComponent*> articulationLinks;
  this->GetOwner()->GetComponents(articulationLinks, false);
  for (auto & link : articulationLinks) {
    //UE_LOG(LogTemp, Warning, TEXT("ArticulationLink : %s"), *link->GetName());
    if (link->GetFName() == name) {
      return link;
    }
  }
  return nullptr;
}

void UArticulationVehicleMovementComponent::CommRecv()
{
  FSimpleMessage rcv;
  if (Comm.RecvLatest(&rcv)) {
    FString type;
    std::string str(TCHAR_TO_UTF8(*rcv.Message));
    std::istringstream buffer(str);
    cereal::JSONInputArchive ar(buffer);
    ar(cereal::make_nvp("CmdType", type));
    if (type == "VW") {
      float v, w;
      ar(cereal::make_nvp("V", v));
      ar(cereal::make_nvp("W", w));
      //UE_LOG(LogTemp, Warning, TEXT("VWCmd V:%f W:%f"), v, w);
      setVW(v, w); // cm/s, deg/s
    }
    if (type == "RPM") {
      float r, l;
      ar(cereal::make_nvp("R", r));
      ar(cereal::make_nvp("L", l));
      //UE_LOG(LogTemp, Warning, TEXT("RpmCmd R:%f L:%f"), r, l);
      setRPM(l, r);
    }
    RemoteAddress = rcv.PeerAddress;
  }
}

void UArticulationVehicleMovementComponent::CommSend()
{
  FVector loc = GetOwner()->GetActorLocation();
  FRotator rot = GetOwner()->GetActorRotation();

  FNamedMessage *msg(new FNamedMessage);
  msg->Message = CerealFromNVP(
    "Position", loc,
    "Pose", Pose,
    "AngVel", RateGyro, //FVector(RollRate, PitchRate, YawRate),
    "Accel", Accel,
    "Yaw", rot.Yaw,
    "LeftRpm", CurRpmLeft,
    "RightRpm", CurRpmRight);
  msg->Name = GetOwner()->GetName();
  Comm.Send(msg);
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
  FString meta;
  {
    meta = FString::Printf(TEXT(
      "\"TreadWidth\":%f,\n"
      "\"ReductionRatio\":%f,\n"
      "\"WheelPerimeterL\":%f,\n"
      "\"WheelPerimeterR\":%f\n"),
      TreadWidth,
      WheelReductionRatio,
      WheelL.Perimeter,
      WheelR.Perimeter
    );
  }
  Comm.setup(GetOwner()->GetFName(), "Vehicle", meta);

  for (TActorIterator<ACommActor> commActorItr(GetWorld()); commActorItr; ++commActorItr) {
    AddTickPrerequisiteActor(*commActorItr);
    break;
  }
}

void UArticulationVehicleMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
  Comm.tearDown();
}

