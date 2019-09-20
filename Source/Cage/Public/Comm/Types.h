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
