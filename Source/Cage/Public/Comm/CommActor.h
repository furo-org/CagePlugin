// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types.h"
#include "MessageEndpoint.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "CommActor.generated.h"


UCLASS()
class CAGE_API ACommActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACommActor();
  virtual ~ACommActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

  void HandleTestMessage(const FTestMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
  void HandleRegisterMessage(const FRegisterMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
  void HandleJsonMessage(const FNamedMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

protected:

  class FImpl;
  FImpl *D = nullptr;

  TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> Endpoint;

};
