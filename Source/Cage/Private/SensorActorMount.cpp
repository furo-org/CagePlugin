// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/


#include "SensorActorMount.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"

bool USensorActorMount::GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)
{
    if(ChildCommMgr)
    {
        if(!ChildCommMgr->GetMetadata(CommMgr, MetaOut))
            return false;
    }

    auto trans=GetComponentTransform().GetRelativeTransform(CommMgr->GetBaseTransform());
    FTransformReport rep;
    rep.Translation=trans.GetTranslation();
    rep.Rotation=trans.GetRotation();
    MetaOut->SetObjectField("Transform-"+GetName(),FJsonObjectConverter::UStructToJsonObject(rep));
    return true;
}

void USensorActorMount::RemoteAddressChanged(const FString& Address)
{
    UE_LOG(LogTemp,Warning,TEXT("SensorActorMount::RemoteAddressChanged %s"),*Address);
    if(ChildCommMgr)
        ChildCommMgr->RemoteAddressChanged(Address);
}

void USensorActorMount::CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>>& RcvJson)
{
    if(ChildCommMgr)
    {
        ChildCommMgr->CommRecv(DeltaTime,RcvJson);
    }
}

TSharedPtr<FJsonObject> USensorActorMount::CommSend(const float DeltaTime, UActorCommMgr* CommMgr)
{
    if(ChildCommMgr)
    {
        return ChildCommMgr->CommSend(DeltaTime, CommMgr);
    }
    return TSharedPtr<FJsonObject>();
}

void USensorActorMount::BeginPlay()
{
    Super::BeginPlay();
    auto child=GetChildActor();
    auto components=child->GetComponents();
    for(UActorComponent *comp:components )
    {
        UActorCommMgr *mgr=Cast<UActorCommMgr>(comp);
        if(mgr)
        {
            ChildCommMgr=mgr;
            UE_LOG(LogTemp,Warning,TEXT("[%s]: Found ActorCommMgr[%s] on [%s]"), *GetName(), *mgr->GetName(), *child->GetName());
            break;
        }
    }
}
