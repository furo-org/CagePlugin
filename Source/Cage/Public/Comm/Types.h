// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Types.generated.h"

USTRUCT()
struct FTestMessage
{
  GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = "Message")
    FString Message;
  FTestMessage() = default;
  FTestMessage(const FString &str):Message(str)
  {}
};

USTRUCT()
struct FAnnounce
{
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, Category = "Message")
  FString Name;
  FAnnounce() = default;
  FAnnounce(const FString &str) :Name(str)
  {}
};

USTRUCT()
struct FRegisterMessage
{
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, Category = "Message")
    FString Name;
  UPROPERTY(EditAnywhere, Category = "Message")
    FString Tag;
  UPROPERTY(EditAnywhere, Category = "Message")
    FString Metadata;
  FRegisterMessage() = default;
  FRegisterMessage(const FString &name, const FString &tag, const FString &meta=FString()) 
    :Name(name),Tag(tag),Metadata(meta)
  {}
};

USTRUCT()
struct FSimpleMessage
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere, Category = "Message")
    FString Message;
  UPROPERTY(EditAnywhere, Category = "Message")
    FString PeerAddress;
};

USTRUCT()
struct FNamedMessage
{
  GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = "Message")
    FString Name;
  UPROPERTY(EditAnywhere, Category = "Message")
    FString Message;
};

USTRUCT()
struct FNamedBinaryData
{
  GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = "Message")
    FString Name;
  UPROPERTY(EditAnywhere, Category = "Message")
    TArray<uint8> Message;
};

// -------------------------------------------------------------------------------------

// Component transform (in actors) representation for metadata 
USTRUCT()
struct CAGE_API FTransformReport
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  FVector Translation;
  UPROPERTY(EditAnywhere)
  FQuat Rotation;
};

// 6 axis inertial status used in SimpleIMU
USTRUCT()
struct CAGE_API FImuReport
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  FVector AngVel; //FVector(RollRate, PitchRate, YawRate),
  UPROPERTY(EditAnywhere)
  FVector Accel;
};

// Ground truth used in PosReporter
USTRUCT()
struct CAGE_API FActorPositionReport
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  FVector Position;  // Actor location
  UPROPERTY(EditAnywhere)
  FQuat Pose;        // Actor pose
  UPROPERTY(EditAnywhere)
  float Yaw;         // Yaw (Heading) component of Pose in degrees
};

//
USTRUCT()
struct CAGE_API FActorGeolocation
{
  GENERATED_BODY()
  UPROPERTY(EditAnywhere)
  FVector Lat;       // Latitude in X: Degrees, Y: Minutes, Z: Seconds 
  UPROPERTY(EditAnywhere)
  FVector Lon;       // Longutude in X: Degrees, Y: Minutes, Z: Seconds 
};

// Current status used in ArticulationMovementComponent
USTRUCT()
struct CAGE_API FVehicleStatus
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  float LeftRpm;   // Left motor speed [rpm]
  UPROPERTY(EditAnywhere)
  float RightRpm;  // Right motor speed [rpm]
};

#if 0
USTRUCT()
struct CAGE_API FActorPosition
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  FVector Position;
  UPROPERTY(EditAnywhere)
  float Yaw;
};
#endif

// Object tracker status
USTRUCT()
struct CAGE_API FTargetActorPosition
{
  GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  FString Name;
  UPROPERTY(EditAnywhere)
  FVector Position;
  UPROPERTY(EditAnywhere)
  bool IsInsight;
};

USTRUCT()
struct CAGE_API FScan2D
{
GENERATED_USTRUCT_BODY()
  UPROPERTY(EditAnywhere)
  TArray<int> Ranges;
  UPROPERTY(EditAnywhere)
  TArray<float> Angles;
};
