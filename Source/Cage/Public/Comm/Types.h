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
