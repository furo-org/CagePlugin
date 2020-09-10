// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
* 
*/

#include "ActorCommMgr.h"
#include "EngineUtils.h"
#include "CommActor.h"
#include "JsonObjectConverter.h"
#include "JsonSerializer.h"

// Sets default values for this component's properties
UActorCommMgr::UActorCommMgr()
{
}


// Called when the game starts
void UActorCommMgr::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("ActorCommMgr: BeginPlay"));
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorCommMgr::InitializeMetadata);
}

void UActorCommMgr::InitializeMetadata()
{
	ActorMetadata.Empty();
	CommClients.Empty();
	TSharedRef<FJsonObject> unifiedmeta=MakeShared<FJsonObject>();
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommMetaSender>(c);
		if(!cli) continue;
		UE_LOG(LogTemp,Verbose,TEXT("[%s] is VehicleCommClient"),*c->GetName());
		TSharedRef<FJsonObject> meta=MakeShared<FJsonObject>();
		auto ok=cli->GetMetadata(this, meta);
		if(!ok)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorCommMgr::InitializeMetadata);
			return;
		}
		if(meta->Values.Num())
		{
			unifiedmeta->Values.Append(meta->Values);
		}
	}
	JsonObjectToFString(unifiedmeta,ActorMetadata);
	Comm.setup(GetOwner()->GetFName(), "Vehicle", ActorMetadata);

	for (TActorIterator<ACommActor> commActorItr(GetWorld()); commActorItr; ++commActorItr)
	{
		AddTickPrerequisiteActor(*commActorItr);
		break;
	}
	// Enable Both PrePhysicsTick and PostPhysicsTick 
	if (!IsTemplate() && PostPhysicsTickFunction.bCanEverTick) {
		UE_LOG(LogTemp, VeryVerbose, TEXT("ActorCommMgr: Enabling PostPhysicsTick"));
		IPostPhysicsTickable::EnablePostPhysicsTick(this);
	}
	if (!IsTemplate() && PrePhysicsTickFunction.bCanEverTick) {
		UE_LOG(LogTemp, VeryVerbose, TEXT("ActorCommMgr: Enabling PrePhysicsTick"));
		IPrePhysicsTickable::EnablePrePhysicsTick(this);
	}

	// Enumerate IActorCommClient components
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommClient>(c);
		if(!cli) continue;
		CommClients.Add(cli);
	}

	// Enumerate IActorCommListener components
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommListener>(c);
		if(!cli) continue;
		CommListeners.Add(cli);
	}
}

void UActorCommMgr::PrePhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	FSimpleMessage rcv;
	TArray<TSharedPtr<FJsonObject>> json;
	if(Comm.Num()==0)return;

	// Receive all ActorCmd messages and merge them 
	while (Comm.Num()!=0)
	{
		Comm.RecvOne(&rcv);
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(*rcv.Message);
		TSharedPtr<FJsonObject> Jo = MakeShared<FJsonObject>();
		FJsonSerializer::Deserialize(JsonReader, Jo);
		json.Add(Jo);
		UE_LOG(LogTemp, Warning, TEXT("CommRecv: %s"),*rcv.Message);
	}
	// Deliver ActorCmd messages to components
	for(const auto cli:CommClients)
	{
		cli->CommRecv(DeltaTime, json);
	}
	// Notify new remote address if it changed
	if(RemoteAddress != rcv.PeerAddress)
	{
		RemoteAddress = rcv.PeerAddress;
		for(const auto cli: CommListeners)
		{
			cli->RemoteAddressChanged(RemoteAddress);
		}
	}
}

void UActorCommMgr::PostPhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	// CommClientのメッセージ群をひとつのjsonにまとめる
	TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
	for(const auto cli:CommClients)
	{
		auto j=cli->CommSend(DeltaTime,this);
		json->Values.Append(j->Values);
	}
	FNamedMessage *msg( new FNamedMessage);
	msg->Name = GetOwner()->GetName();
	if(JsonObjectToFString(json,msg->Message))
	{
		Comm.Send(msg);
	}
}

FTransform UActorCommMgr::GetBaseTransform()
{
    if(!BaseSocket.IsNone() && !GetOwner()->GetRootComponent()->DoesSocketExist(BaseSocket))
    {
	    UE_LOG(LogTemp, Error, TEXT("UActorCommMgr[%s]::GetBaseTransform: No base socket named [%s] found"),*BaseSocket.ToString());
    }
    return GetOwner()->GetRootComponent()->GetSocketTransform(BaseSocket);
}
