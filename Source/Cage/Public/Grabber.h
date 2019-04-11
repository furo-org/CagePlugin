// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grabber.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CAGE_API UGrabber : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrabber();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

  UPROPERTY(EditDefaultsOnly)
  float ReachDistance = 200;
  UPROPERTY(EditDefaultsOnly)
  float GrabRadialTolelance = 2;
  UPROPERTY(EditDefaultsOnly)
  float GrabRadialStep = 60;
  UPROPERTY(BlueprintReadOnly)
  bool Selecting = false;

  float GrabVDir = 0;
  UPROPERTY(BlueprintReadOnly)
    FVector GrabStart;
  UPROPERTY(BlueprintReadOnly)
    FVector GrabVec;

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

  UFUNCTION(BlueprintCallable)
    void SelectStart();
  UFUNCTION(BlueprintCallable)
    void SelectEnd();

  UFUNCTION(BlueprintCallable)
    void Grab();
  UFUNCTION(BlueprintCallable)
    void UnGrab();
  UFUNCTION(BlueprintPure)
    bool isGrabbing();
  UFUNCTION(BlueprintPure)
    bool isSelecting();

  UFUNCTION(BlueprintCallable)
    void TiltGrabber(float relativePitch);

  UFUNCTION(BlueprintCallable)
    void HilightActor(AActor* actor);
  UFUNCTION(BlueprintCallable)
    void Unhilight();

private:
  void SelectObject();

  FVector GrabberVector(int n);// Generate n-th grabend vector

  AActor* GetReachableObject();
  TWeakObjectPtr<UPrimitiveComponent> HilightedComponent=nullptr;
  TWeakObjectPtr<UPrimitiveComponent> GrabbedComponent=nullptr;
  bool GrabObjectNeedsPhysics;
  bool GrabObjectNeedsGravity;
  bool GrabObjectNeedsCollision;
};
