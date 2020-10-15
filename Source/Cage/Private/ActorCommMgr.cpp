// Copyright 2020 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
* 
*/

#include "Comm/ActorCommMgr.h"
#include "EngineUtils.h"
#include "Comm/CommActor.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Comm/Comm.h"

class UActorCommMgr::FImpl{
	public:
	CommEndpointIO<FSimpleMessage> Comm;
	TArray<IActorCommClient*> CommClients;
	TArray<IActorCommListener*> CommListeners;
	TSharedPtr<FJsonObject> ActorMetadata;
	AActor *ParentActor=nullptr;
	bool IsReady=false;
};


// Sets default values for this component's properties
UActorCommMgr::UActorCommMgr()
{
}


// Called when the game starts
void UActorCommMgr::BeginPlay()
{
	Super::BeginPlay();
	D=new FImpl();
	UE_LOG(LogTemp, Warning, TEXT("ActorCommMgr: BeginPlay"));
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorCommMgr::Initialize);
	for(auto *c:GetOwner()->GetComponents())
	{
		if(Cast<USkeletalMeshComponent>(c))
		{
			BaseComp=Cast<USceneComponent>(c);
		}
	}
	if(!BaseComp)
	{
		BaseComp=GetOwner()->GetRootComponent();
	}
	ensure(BaseComp!=nullptr);
	if(BaseSocket!=NAME_None && !BaseComp->DoesSocketExist(BaseSocket))
	{
		UE_LOG(LogTemp,Warning,TEXT("[%1:%2] : BaseSocket[%3] is not found"),*GetOwner()->GetName(), *GetName(), *BaseSocket.ToString());
	}
}

void UActorCommMgr::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if(D!=nullptr)
	{
		delete D;
		D=nullptr;		
	}
}

void UActorCommMgr::Initialize()
{
	D->ParentActor=GetOwner()->GetAttachParentActor();
	if(D->ParentActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] parentActor=%s"),*GetOwner()->GetName(), *D->ParentActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] GetAttachParentActor returns nullptr"),*GetOwner()->GetName());
	}

	if(!CollectMetadata())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorCommMgr::Initialize);
		return;
	}
	if(D->ParentActor==nullptr)
	{
		FString metastr;
		if(D->ActorMetadata.IsValid())
		{
			JsonObjectToFString(D->ActorMetadata.ToSharedRef(), metastr);
			D->Comm.setup(GetOwner()->GetFName(), "Vehicle", metastr);
		}
	}

	for (TActorIterator<ACommActor> commActorItr(GetWorld()); commActorItr; ++commActorItr)
	{
		AddTickPrerequisiteActor(*commActorItr);
		break;
	}
	// Enable Both PrePhysicsTick and PostPhysicsTick 
	if (!IsTemplate() && PostPhysicsTickFunction.bCanEverTick && D->ParentActor==nullptr) {
		UE_LOG(LogTemp, VeryVerbose, TEXT("ActorCommMgr: Enabling PostPhysicsTick"));
		IPostPhysicsTickable::EnablePostPhysicsTick(this);
	}
	if (!IsTemplate() && PrePhysicsTickFunction.bCanEverTick && D->ParentActor==nullptr) {
		UE_LOG(LogTemp, VeryVerbose, TEXT("ActorCommMgr: Enabling PrePhysicsTick"));
		IPrePhysicsTickable::EnablePrePhysicsTick(this);
	}

	// Enumerate IActorCommClient components
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommClient>(c);
		if(!cli) continue;
		if(cli==this) continue;
		D->CommClients.Add(cli);
	}

	// Enumerate IActorCommListener components
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommListener>(c);
		if(!cli) continue;
		if(cli==this) continue;
		D->CommListeners.Add(cli);
	}
	D->IsReady=true;
}

bool UActorCommMgr::CollectMetadata()
{
	D->CommClients.Empty();
	TSharedRef<FJsonObject> unifiedmeta=MakeShared<FJsonObject>();
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommMetaSender>(c);
		if(!cli) continue;
		if(cli==this) continue;
		TSharedRef<FJsonObject> meta=MakeShared<FJsonObject>();
		auto ok=cli->GetMetadata(this, meta);
		if(!ok)
		{
			return false;
		}
		if(meta->Values.Num())
		{
			unifiedmeta->Values.Append(meta->Values);
		}
	}
	D->ActorMetadata=unifiedmeta;
	return true;
}

void UActorCommMgr::PrePhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	FSimpleMessage rcv;
	TArray<TSharedPtr<FJsonObject>> json;
	if(D->Comm.Num()==0)return;

	// Receive all ActorCmd messages and merge them 
	while (D->Comm.Num()!=0)
	{
		D->Comm.RecvOne(&rcv);
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(*rcv.Message);
		TSharedPtr<FJsonObject> Jo = MakeShared<FJsonObject>();
		FJsonSerializer::Deserialize(JsonReader, Jo);
		json.Add(Jo);
		//UE_LOG(LogTemp, Warning, TEXT("CommRecv: %s"),*rcv.Message);
	}
	// Deliver ActorCmd messages to components
	for(const auto cli:D->CommClients)
	{
		cli->CommRecv(DeltaTime, json);
	}
	// Notify new remote address if it changed
	if(RemoteAddress != rcv.PeerAddress)
	{
		RemoteAddress = rcv.PeerAddress;
		RemoteAddressChanged(rcv.PeerAddress);
	}
}

void UActorCommMgr::PostPhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	// CommClientのメッセージ群をひとつのjsonにまとめる
	TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
	for(const auto cli: D->CommClients)
	{
		auto j=cli->CommSend(DeltaTime,this);
		if(j)
		{
			json->Values.Append(j->Values);
		}			
	}
	FNamedMessage *msg( new FNamedMessage);
	msg->Name = GetOwner()->GetName();
	if(JsonObjectToFString(json,msg->Message))
	{
		D->Comm.Send(msg);
	}
}

FTransform UActorCommMgr::GetBaseTransform()
{
    return BaseComp->GetSocketTransform(BaseSocket);
}

bool UActorCommMgr::GetMetadata(UActorCommMgr* CommMgr, TSharedRef<FJsonObject> MetaOut)
{
	if(!D->IsReady) return false;
	if(!D->ActorMetadata.IsValid()) return false;
	MetaOut=D->ActorMetadata.ToSharedRef();
	return true;
}

void UActorCommMgr::RemoteAddressChanged(const FString& Address)
{
	for(const auto cli: D->CommListeners)
	{
		cli->RemoteAddressChanged(Address);
	}
}
