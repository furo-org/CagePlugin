// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ObjectTracker.h"

#include "EngineUtils.h"
#include "Comm/Types.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"


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
  for(APawn* pawn: TActorRange<APawn>(GetWorld()))
  {
    if (!pawn->IsInLevel(GetWorld()->GetCurrentLevel()))continue;
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
    GetWorld()->LineTraceSingleByChannel(resHit, traceStart, traceEnd, ECC_Visibility, params, resParams);
    FString Result;
    FTargetActorPosition Data;
    //  Hitしたらそのオブジェクトの位置を取得してCommにSend
    // 古いバージョンでは次のような json objectが戻されたが、現在は変更している
    //  "TargetName": pawn->GetName(),
    //  "TargetPos": pawn->GetActorLocation(),
    //  "IsInsight": true/false
    Data.Name=pawn->GetName();
    Data.Position=pawn->GetActorLocation();
    if (resHit.GetActor() == pawn)
    {
      Data.IsInsight=true;
    }
    else
    {
      Data.IsInsight=false;
      UE_LOG(LogTemp, Warning, TEXT("Out of sight %s"), *pawn->GetName());
    }
    auto Msg = new FNamedMessage;
    Msg->Name = GetName();
    FJsonObjectConverter::UStructToJsonObjectString(Data, Msg->Message);
    Comm.Send(Msg);
  }
}
