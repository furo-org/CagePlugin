// Fill out your copyright notice in the Description page of Project Settings.

#include "PosReporter.h"
#include "MessageEndpointBuilder.h"
#include "IMessageContext.h"
#include "AllowWindowsPlatformTypes.h"
#include "zmq_nt.hpp"
#include "HideWindowsPlatformTypes.h"
#include "Types.h"
// cerial-UE4
#include "cereal-UE4.hxx"

// Sets default values for this component's properties
UPosReporter::UPosReporter()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPosReporter::BeginPlay()
{
  Super::BeginPlay();

  UE_LOG(LogTemp, Warning, TEXT("BeginPlay "));
  Comm.setup(GetOwner()->GetFName());
}

void UPosReporter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Comm.tearDown();
}


// Called every frame
void UPosReporter::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  FVector loc=GetOwner()->GetActorLocation();
  FRotator rot = GetOwner()->GetActorRotation();

  FNamedMessage *msg(new FNamedMessage);
  msg->Message = CerealFromNVP("Position", loc, "Yaw", rot.Yaw);
  msg->Name = GetOwner()->GetName();
  Comm.Send(msg);
  //UE_LOG(LogTemp, Warning, TEXT("PositionReporter: Tick "));
}
