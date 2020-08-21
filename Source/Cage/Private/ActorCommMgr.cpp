// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorCommMgr.h"
#include "EngineUtils.h"
#include "CommActor.h"

// Sets default values for this component's properties
UActorCommMgr::UActorCommMgr()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
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
	for(const auto c:GetOwner()->GetComponents())
	{
		auto cli=Cast<IActorCommClient>(c);
		UE_LOG(LogTemp,Warning,TEXT("InitializeMetadata: [%s]"),*c->GetName());
		if(!cli) continue;
		UE_LOG(LogTemp,Warning,TEXT("[%s] is VehicleCommClient"),*c->GetName());
		FString meta;
		auto ok=cli->GetMetadata(meta);
		if(!ok)
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UActorCommMgr::InitializeMetadata);
			return;
		}
		ActorMetadata.Append(meta);
		CommClients.Add(cli);
		if(!meta.EndsWith("\n"))
		{
			ActorMetadata.Append("\n");
		}
	}

	Comm.setup(GetOwner()->GetFName(), "Vehicle", ActorMetadata);
	for (TActorIterator<ACommActor> commActorItr(GetWorld()); commActorItr; ++commActorItr)
	{
		AddTickPrerequisiteActor(*commActorItr);
		break;
	}
	if (!IsTemplate() && PostPhysicsTickFunction.bCanEverTick) {
		UE_LOG(LogTemp, Warning, TEXT("ActorCommMgr: Enabling PostPhysicsTick"));
		IPostPhysicsTickable::EnablePostPhysicsTick(this);
	}
	if (!IsTemplate() && PrePhysicsTickFunction.bCanEverTick) {
		UE_LOG(LogTemp, Warning, TEXT("ActorCommMgr: Enabling PrePhysicsTick"));
		IPrePhysicsTickable::EnablePrePhysicsTick(this);
	}
}


// Called every frame
void UActorCommMgr::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UActorCommMgr::PrePhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	// CommActorからメッセージ群を受信してひとつのjsonにまとめる
	FSimpleMessage rcv;
	TArray<TSharedPtr<FJsonObject>> json;
	while (Comm.Num()!=0)
	{
		Comm.RecvOne(&rcv);
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(*rcv.Message);
		TSharedPtr<FJsonObject> Jo = MakeShared<FJsonObject>();
		FJsonSerializer::Deserialize(JsonReader, Jo);
		json.Add(Jo);
		UE_LOG(LogTemp, Warning, TEXT("CommRecv: %s"),*rcv.Message);
	}
	for(const auto cli:CommClients)
	{
		cli->CommRecv(DeltaTime, json);
	}
	RemoteAddress = rcv.PeerAddress;			
}

void UActorCommMgr::PostPhysicsTick(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	FNamedMessage *msg( new FNamedMessage);
	for(const auto cli:CommClients)
	{
		auto m=cli->CommSend(DeltaTime);
		if(!m.EndsWith("\n"))
			m.Append("\n");
		msg->Message.Append(m);
	}
	msg->Name = GetOwner()->GetName();
	Comm.Send(msg);
}
