// Fill out your copyright notice in the Description page of Project Settings.

#include "Grabber.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
UGrabber::UGrabber()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGrabber::BeginPlay()
{
	Super::BeginPlay();
  GrabVDir = 0;
}


// Called every frame
void UGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


  GrabStart = GetOwner()->GetActorLocation();
  auto pcItr=GetWorld()->GetPlayerControllerIterator();
  if (pcItr) {
    auto viewRot = (*pcItr)->GetControlRotation();
    GrabVec = FRotator(viewRot.Pitch, viewRot.Yaw, 0).RotateVector(FVector(1, 0, 0));
  }
  else {
    GrabVec = FRotator(GrabVDir, GetOwner()->GetActorRotation().Yaw, 0).RotateVector(FVector(1, 0, 0));
  }

#if 0
  for (int i = 0; i < 10; ++i) {
    FVector ge = GrabendVector(i);
    DrawDebugLine(GetWorld(),
      GrabStart,
      GrabStart + ge,
      FColor(0, 100, 250), false, 0.f, 0.f, 0.5f);
  }
#endif

  if (Selecting) SelectObject();
  if (isGrabbing() == false)return;
  //UE_LOG(LogTemp, Warning, TEXT("moving actor : %s"), *GrabbedComponent->GetOwner()->GetName());
  GrabbedComponent->GetOwner()->SetActorLocationAndRotation(GrabStart + GrabVec * ReachDistance*0.5, FRotator(), false, nullptr);
}

void UGrabber::SelectStart()
{
  Selecting = true;
}

void UGrabber::SelectEnd()
{
  Selecting = false;
  Unhilight();
}

bool UGrabber::isSelecting()
{
  return Selecting;
}

UPrimitiveComponent* getComponentFromActor(AActor* actor) {
  if (actor != nullptr) {
    //if (hit.GetComponent() != nullptr) return hit.GetComponent();
    return actor->FindComponentByClass<UPrimitiveComponent>();
  }
  return nullptr;
}

void UGrabber::SelectObject() {
  DrawDebugLine(GetWorld(),
    GrabStart,
    GrabStart + GrabVec*ReachDistance,
    FColor(0, 250, 0), false, 0.f, 0.f, 0.5f);

  //UE_LOG(LogTemp, Warning, TEXT("Grab Start: %s  End: %s"), *GrabStart.ToString(), *(GrabStart + GrabVec * ReachDistance).ToString());

  AActor* actor = GetReachableObject();
  HilightActor(actor);
  return;
}

void UGrabber::HilightActor(AActor* actor)
{
  Unhilight();
  UPrimitiveComponent *comp = getComponentFromActor(actor);
  if (comp == nullptr)return;
  HilightedComponent = comp;
  if (HilightedComponent != nullptr) {
    HilightedComponent->SetRenderCustomDepth(true);
  }
}

void UGrabber::Unhilight() {
  if (HilightedComponent != nullptr)HilightedComponent->SetRenderCustomDepth(false);
}

void UGrabber::Grab()
{
  UPrimitiveComponent *comp = getComponentFromActor(GetReachableObject());
  if (comp!=nullptr) {
    GrabbedComponent = comp;
    GrabObjectNeedsCollision = comp->GetOwner()->GetActorEnableCollision();
    GrabObjectNeedsPhysics = comp->IsSimulatingPhysics();
    GrabObjectNeedsGravity = comp->IsGravityEnabled();
    //comp->SetSimulatePhysics(false);
    comp->SetEnableGravity(false);
    comp->GetOwner()->SetActorEnableCollision(false);
    UE_LOG(LogTemp, Warning, TEXT("Grab hit Actor:%s Comp:%s Coll:%d "), *comp->GetOwner()->GetName(), *comp->GetName(), comp->GetOwner()->GetActorEnableCollision());
  }
  else {
    UE_LOG(LogTemp, Warning, TEXT("Nothing to grab"));
  }

}

void UGrabber::UnGrab()
{
  if (isGrabbing()) {
    //GrabbedComponent->SetSimulatePhysics(GrabObjectNeedsPhysics);
    GrabbedComponent->SetEnableGravity(GrabObjectNeedsGravity);
    GrabbedComponent->GetOwner()->SetActorEnableCollision(GrabObjectNeedsCollision);
    GrabbedComponent.Reset();
  }
}

bool UGrabber::isGrabbing()
{
  if (GrabbedComponent == nullptr)return false;
  return true;
}

void UGrabber::TiltGrabber(float relativePitch)
{
  GrabVDir = FMath::Clamp(GrabVDir + relativePitch, -60.f, 60.f);;
}

AActor* UGrabber::GetReachableObject()
{
  for (int i = 0; i < 16; ++i) {
    FHitResult hit;
    FCollisionQueryParams queryParams(FName(), false, GetOwner());
    FCollisionObjectQueryParams objectQueryParams(ECC_PhysicsBody);
    objectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
    GetWorld()->LineTraceSingleByObjectType(
      hit,
      GrabStart,
      GrabStart + GrabberVector(i),
      objectQueryParams,
      queryParams
    );
#if 0
    DrawDebugLine(GetWorld(),
      GrabStart,
      GrabStart + GrabberVector(i),
      FColor(100, 250, 0), false, 0.f, 0.f, 0.5f);
#endif
    if(hit.GetActor())
      return hit.GetActor();
  }
  return nullptr;
}

FVector UGrabber::GrabberVector(int n) {

  // radial vector
  FVector radial = FVector::VectorPlaneProject(FVector(0, 0, 1), GrabVec);
  radial.Normalize();

  // n==0 -> GrabVec
#if 0
  UE_LOG(LogTemp,Warning,TEXT("GrabendVector [%d] ang: %f , tol: %f , up: %s"),
    n, GrabRadialStep*n, GrabRadialTolelance*n,*radial.ToString()
    )
#endif
  return GrabVec * ReachDistance + radial.RotateAngleAxis(GrabRadialStep * n, GrabVec) * GrabRadialTolelance * n;
}
