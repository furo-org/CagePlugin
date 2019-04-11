// Fill out your copyright notice in the Description page of Project Settings.

#include "ObjectTracker.h"
// cerial-UE4
#include "cerealUE4.hh"


// Sets default values for this component's properties
UObjectTracker::UObjectTracker()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UObjectTracker::BeginPlay()
{
	Super::BeginPlay();

  Comm.setup(FName(NAME_None));
}

void UObjectTracker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Comm.tearDown();
}


// Called every frame
void UObjectTracker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  int count = 0;
	// 追跡対象オブジェクトをリストアップ
  for (FConstPawnIterator pitr = GetWorld()->GetPawnIterator(); pitr; ++pitr)
  {
    const TWeakObjectPtr<APawn> pawn = *pitr;
    count++;
    bool match = false;
    for (auto &tag : pawn->Tags) {
      if (TargetTags.Contains(tag)) {
        match = true;
        break;
      }
    }
    if (!match)continue;

    // 追跡対象の各オブジェクトについて
    //  LineTraceし、Hitするかをチェック
    FHitResult resHit;
    FVector target = pawn->GetActorLocation();
    FVector traceStart = GetOwner()->GetActorLocation();
    target.Z = traceStart.Z;
    float distance = (target - traceStart).Size();
    FVector traceEnd = target;
    if(distance>MaxRange){
      FVector traceVec = target - traceStart;
      traceEnd = traceStart + traceVec.GetSafeNormal()*MaxRange;
    }

    FCollisionResponseParams resParams;
    FCollisionQueryParams params;
    params.bTraceComplex = true;
    params.AddIgnoredActor(GetOwner());
    bool res = GetWorld()->LineTraceSingleByChannel(resHit, traceStart, traceEnd, ECC_Visibility, params, resParams);
    FString Result;
    if (resHit.GetActor() == pawn)
    {
      //  Hitしたらそのオブジェクトの位置を取得してCommにSend
      Result = CerealFromNVP(
        "TargetName", pawn->GetName(),
        "TargetPos", pawn->GetActorLocation(),
        "IsInsight", true);
    }
    else
    {
      Result = CerealFromNVP(
        "TargetName", pawn->GetName(),
        "TargetPos", pawn->GetActorLocation(),
        "IsInsight", false);
      UE_LOG(LogTemp, Warning, TEXT("Out of sight %s"), *pawn->GetName());
    }
    auto msg = new FNamedMessage;
    msg->Name = GetName();
    msg->Message = Result;
    Comm.Send(msg);
  }
}
