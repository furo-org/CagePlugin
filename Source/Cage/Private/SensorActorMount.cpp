// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/


#include "SensorActorMount.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"

bool USensorActorMount::GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)
{
    //auto trans=GetComponentTransform().GetRelativeTransform(GetOwner()->GetTransform());
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
}
