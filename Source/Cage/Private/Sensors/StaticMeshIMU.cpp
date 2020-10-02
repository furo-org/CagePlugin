// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include "StaticMeshIMU.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"

// Called when the game starts
void UStaticMeshIMU::BeginPlay()
{
	Super::BeginPlay();

	// ...
	PrevTransform=GetComponentTransform();
	PrevVelocity=GetOwner()->GetVelocity();
}

bool UStaticMeshIMU::GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)
{
	auto trans=GetComponentTransform().GetRelativeTransform(CommMgr->GetBaseTransform());
	FTransformReport rep;
	rep.Translation=trans.GetTranslation();
	rep.Rotation=trans.GetRotation();
	MetaOut->SetObjectField("Transform-"+GetName(),FJsonObjectConverter::UStructToJsonObject(rep));
	return true;
}

void UStaticMeshIMU::CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>>& Json)
{
}

TSharedPtr<FJsonObject> UStaticMeshIMU::CommSend(const float DeltaTime, UActorCommMgr *CommMgr)
{
	FTransform currentTransform = GetComponentTransform();
	FTransform deltaTransform = currentTransform.GetRelativeTransform(PrevTransform);

	auto deltaRotation = deltaTransform.GetRotation().Euler() / DeltaTime;
	RateGyro = GyroEMACoeff * deltaRotation + (1 - GyroEMACoeff) * RateGyro;
	float transformPitch = deltaRotation.Y;
	float transformRoll = deltaRotation.X;
	float transformYaw = deltaRotation.Z;

	PitchRate = transformPitch;
	RollRate = transformRoll;
	YawRate = transformYaw;

	PrevTransform = currentTransform;
	FVector velVec;
	if(IsSimulatingPhysics()){
		velVec = GetComponentVelocity();
	}else{
		velVec=deltaTransform.GetTranslation() / DeltaTime;
	}
	auto accelVec = (velVec - PrevVelocity) / DeltaTime;
	auto accel = currentTransform.GetRotation().UnrotateVector(accelVec + FVector(0, 0, -GetWorld()->GetGravityZ()));
	Accel = AccelEMACoeff * accel + (1.0 - AccelEMACoeff) * Accel;
	//UE_LOG(LogTemp, Warning, TEXT("[%s] dt: %f Vel: %f %f %f   Accel: %f %f %f" (EMA:%f), *(this->GetName()), DeltaTime, velVec.X, velVec.Y, velVec.Z, Accel.X, Accel.Y, Accel.Z, AccelEMACoeff);
	PrevVelocity = velVec;
	//UE_LOG(LogTemp, Warning, TEXT("[%s] Physics enabled"), *(this->GetName()));

#if 0
	FVector lin, ang;
	vehicleRoot->GetWorldVelocity(lin, ang);
	UE_LOG(LogTemp, Warning, TEXT("[%s] lin: %f %f %f   ang: %f %f %f"),*(this->GetName()),
      lin.X, lin.Y, lin.Z,  ang.X, ang.Y, ang.Z);
#endif

	FImuReport ir;
	ir.AngVel = RateGyro; //FVector(RollRate, PitchRate, YawRate),
	ir.Accel = Accel;
	return FJsonObjectConverter::UStructToJsonObject(ir);
}
