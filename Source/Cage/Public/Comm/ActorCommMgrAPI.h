// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#pragma once
#include "CoreMinimal.h"
#include "ActorCommMgrApi.generated.h"

UINTERFACE()
class UActorCommClient : public UInterface {
    GENERATED_BODY()
public:
};

class UActorCommMgr;

class IActorCommClient
{
    GENERATED_BODY()
public:
    virtual void CommRecv(const float DeltaTime, const TArray<TSharedPtr<FJsonObject>> &RcvJson){};
    virtual TSharedPtr<FJsonObject> CommSend(const float DeltaTime, UActorCommMgr *CommMgr){return TSharedPtr<FJsonObject>();};
};

UINTERFACE()
class UActorCommListener : public UInterface {
    GENERATED_BODY()
public:
};

class IActorCommListener
{
    GENERATED_BODY()
public:
    virtual void RemoteAddressChanged(const FString &Address)=0;
};

UINTERFACE()
class UActorCommMetaSender : public UInterface {
    GENERATED_BODY()
public:
};

class IActorCommMetaSender
{
    GENERATED_BODY()
public:
    virtual bool GetMetadata(UActorCommMgr *CommMgr, TSharedRef<FJsonObject> MetaOut)=0;
};
