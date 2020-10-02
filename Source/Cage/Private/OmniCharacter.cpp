// Fill out your copyright notice in the Description page of Project Settings.


#include "OmniCharacter.h"
#include "OmniMovementComponent.h"

// Sets default values
AOmniCharacter::AOmniCharacter(const FObjectInitializer& ObjectInitializer):
Super(ObjectInitializer.SetDefaultSubobjectClass<UOmniMovementComponent>(TEXT("CharMoveComp")))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOmniCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOmniCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AOmniCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

