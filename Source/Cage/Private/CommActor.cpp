// Fill out your copyright notice in the Description page of Project Settings.

#include "CommActor.h"
#include "MessageEndpointBuilder.h"
#include "IMessageContext.h"
#include "AllowWindowsPlatformTypes.h"
#include "zmq_nt.hpp"
#include "HideWindowsPlatformTypes.h"
#include "Types.h"
#include <sstream>
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Math/TransformCalculus2D.h" // need this in addition to CoreMinimal.h to compile cereal-UE4.hxx
#include "cereal-UE4.hxx"

// Cereal serealize rules
#if 0
template<typename Archive>
void serialize(Archive &ar, FNamedMessage j) {
  ar(
    cereal::make_nvp("Name", j.Name),
    cereal::make_nvp("Data", j.Message)
  );
}
template<typename Archive>
void serialize(Archive &ar, FNamedBinaryData b) {
  ar(
    cereal::make_nvp("Name", b.Name),
    cereal::make_nvp("Data", b.Message)
  );
}
#endif
class ACommActor::FImpl {
public:
  zmq::context_t *ZContext = nullptr;
  zmq::socket_t *ZReportSocket = nullptr;
  zmq::socket_t *ZCmdSocket = nullptr;
  bool SocketsReady = false;

  FImpl() {
    UE_LOG(LogTemp, Warning, TEXT("ACommActor::FImpl created"));
    int err = 0;
    ZContext = new zmq::context_t(1);
    if (ZContext && ZContext->isValid()) {
      UE_LOG(LogTemp, Warning, TEXT("ZContext ready"));
      ZReportSocket = new zmq::socket_t(*ZContext, ZMQ_PUB);
      ZCmdSocket = new zmq::socket_t(*ZContext, ZMQ_REP);
      if (!ensure(ZReportSocket != nullptr && ZReportSocket->isValid())) {
        UE_LOG(LogTemp, Warning, TEXT("Unable to create ZSocket for report announcer"));
        return;
      }
      if (!ensure(ZCmdSocket != nullptr && ZCmdSocket->isValid())) {
        UE_LOG(LogTemp, Warning, TEXT("Unable to create ZSocket for command receiver"));
        return;
      }
      int timeout = 500;
      ZReportSocket->setsockopt(ZMQ_RCVTIMEO, timeout);
      ZReportSocket->setsockopt(ZMQ_SNDTIMEO, timeout);
      ZCmdSocket->setsockopt(ZMQ_RCVTIMEO, timeout);
      ZCmdSocket->setsockopt(ZMQ_SNDTIMEO, timeout);

      UE_LOG(LogTemp, Warning, TEXT("ZSocket binding to tcp://*:54321"));
      if ((err = ZReportSocket->bind("tcp://*:54321")) != 0) {
        UE_LOG(LogTemp, Error, TEXT("Could not bind ReportSocket : %d [%s]"), err, ANSI_TO_TCHAR(zmq_strerror(err)));
      }
      else if ((err = ZCmdSocket->bind("tcp://*:54323")) != 0) {
        UE_LOG(LogTemp, Error, TEXT("Could not bind CmdSocket : %d [%s]"), err, ANSI_TO_TCHAR(zmq_strerror(err)));
      }else{
        UE_LOG(LogTemp, Warning, TEXT("ReportSocket (tcp://*:54321) and CmdSocket (tcp://*:54323) are ready"));
        SocketsReady = true;
      }
    }
    else {
      UE_LOG(LogTemp, Warning, TEXT("ZContext is NOT ready"));
    }
  }
  ~FImpl() {
    if (ZCmdSocket) {
      delete ZCmdSocket;
      ZCmdSocket = nullptr;
    }
    if (ZReportSocket) {
      delete ZReportSocket;
      ZReportSocket = nullptr;
    }
    if (ZContext) {
      delete ZContext;
      ZContext = nullptr;
    }
    SocketsReady = false;
  }
protected:
  // Endpoint repository
  //   name, tag, endpoint
  // access pattern
  //   enumerate_by_tag     -> list of names
  //   get_endpoint_by_name
  //   register_endpoint
  //     name            endpoint         tag
  TMap<FString, TTuple<FMessageAddress, FString, FString> > EndPoints;
public:
  // tagNameのタグがついたendpointの名前リストを取得する
  TArray<FString> EnumerateByTag(FString tagName) {
    TArray<FString> ret;
    for (const auto& Entry : EndPoints)
    {
      FString tag = Entry.Value.Get<1>();
      if (tag == tagName) {
        ret.Add(Entry.Key);
      }
    }
    return ret;
  }
  FMessageAddress GetEndpointByName(FString objectName) {
    auto *entry = EndPoints.Find(objectName);
    if (entry == nullptr)return FMessageAddress();
    return entry->Get<0>();
  }
  FString GetMetaByName(FString objectName) {
    auto *entry = EndPoints.Find(objectName);
    if (entry == nullptr)return FString();
    return entry->Get<2>();
  }
  void RegisterEndpoint(FString objectName, FMessageAddress endpoint, FString tag, FString meta=FString()) {
    EndPoints.Emplace(objectName, MakeTuple(endpoint, tag, meta));
    //UE_LOG(LogTemp, Warning, TEXT("RegisterEndpoint: name=%s, tag=%s meta=%s"), *objectName, *tag, *meta);
  }
};


// Sets default values
ACommActor::ACommActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

//ACommActor::ACommActor(ACommActor&& rhs) = default;

//ACommActor::ACommActor& ACommActor::operator=(ACommActor&& rhs) = default;

ACommActor::~ACommActor() {}

// Called when the game starts or when spawned
void ACommActor::BeginPlay()
{
	Super::BeginPlay();
	
  Endpoint = FMessageEndpoint::Builder("CommActor")
    .Handling<FTestMessage>(this, &ACommActor::HandleTestMessage)
    .Handling<FRegisterMessage>(this, &ACommActor::HandleRegisterMessage)
    .Handling<FNamedMessage>(this, &ACommActor::HandleJsonMessage);

  if (Endpoint.IsValid()) {
    Endpoint->Subscribe<FTestMessage>();
    Endpoint->Subscribe<FNamedMessage>();
    UE_LOG(LogTemp, Warning, TEXT("Message endpoint ready: %s"), *Endpoint->GetAddress().ToString());
  }
  D = new FImpl;
 }

void ACommActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);
  Endpoint.Reset();
  if (D != nullptr) {
    delete D;
    D = nullptr;
  }
}

// Called every frame
void ACommActor::Tick(float DeltaTime)
{
  static int modulo = 0;

	Super::Tick(DeltaTime);

  if (modulo % 100 == 0) {
    if (Endpoint.IsValid()) {
      Endpoint->Publish(new FAnnounce(GetName()));
    }
  }
/*
Message の構造として期待するもの

  Console Command
  {
    Type: "Console",
    Input: "server travel hogeMap"
  }
  Console Commandの実行結果が返る

  Query Endpoints
  {
    Type: "ListEndpoint",
    Tag: "Vehicle"
  }
  指定Tagを持つEndpointのリストが返る

  Query Actor
  {
    Type: "GetActorMeta",
    "Endpoint": "Puffin_BP_0"
  }

  Vehicle Command
  {
    "Type": "ActorCmd",
    "Endpoint": "Puffin_BP_0"
    }
  }
  -----
  ....
  ActorCmdはマルチパートで送る。2つめのパートがActorに送られる。
  送るActorが存在したかどうかのステータスが返る。

*/
  if (D->SocketsReady)
  {
    zmq::message_t msg, msgbody;
    int ret = D->ZCmdSocket->recv(&msg, ZMQ_DONTWAIT);
    if (ret>=0) {
      //UE_LOG(LogTemp, Warning, TEXT("message (len=%d) received"), ret);

      if (msg.more()) {
        // DONTWAITは不要? 非同期にする?
        ret = D->ZCmdSocket->recv(&msgbody, ZMQ_RCVMORE);
        //UE_LOG(LogTemp, Warning, TEXT("second part message (len=%d) received"), ret);
      }

      FString type;
      std::string str(msg.data<char>(),msg.size());
      std::istringstream buffer(str);
      cereal::JSONInputArchive ar(buffer);
      ar(cereal::make_nvp("Type", type));
      FString retmsg;

      if (type == "Console") {
        FString line;
        ar(cereal::make_nvp("Input", line));
        UE_LOG(LogTemp, Warning, TEXT("Console command requested: %s"),*line);
        FStringOutputDevice outbuff;
        GEngine->Exec(GetWorld(), *line, outbuff);
        retmsg=FString::Printf(TEXT(
          "{\n"
          " \"Type\" : \"Console\",\n"
          " \"Result\" : \"%s\"\n"
          "}")
          ,*outbuff.ReplaceQuotesWithEscapedQuotes()
        );
      }
      else if (type == "ListEndpoint") {
        FString tag;
        ar(cereal::make_nvp("Tag", tag));
        auto eps = D->EnumerateByTag(tag);
        FString epArray;
        for (const auto &ep : eps) {
          if (!epArray.IsEmpty())epArray += ",";
          epArray += FString::Printf(TEXT("\"%s\""), *ep);
        }
        retmsg = FString::Printf(TEXT(
          "{\n"
          " \"Type\" : \"ListEndpoint\",\n"
          " \"Result\" : [%s]\n"
          "}"
        ), *epArray);
      }
      else if (type == "GetActorMeta") {
        FString target;
        FString resultCode;
        ar(cereal::make_nvp("Endpoint", target));
        if (target.IsEmpty()) {
          resultCode = "No endpoint specified.";
        }
        else
        {
          auto meta = D->GetMetaByName(target);
          UE_LOG(LogTemp, Warning, TEXT("GetActorMeta[%s]: Q:%s meta:%s"), *GetName(),*target, *meta);
          retmsg = FString::Printf(TEXT(
            "{\n"
            " \"Type\" : \"GetActorMeta\",\n"
            " \"Result\" : {\n%s\n}\n"
            "}"
          ), *meta);
        }
      }
      else if (type == "ActorMsg") {
        FString target;
        FString resultCode;
        ar(cereal::make_nvp("Endpoint", target));
        if (target.IsEmpty()) {
          resultCode = "No endpoint specified.";
        }
        else {
          auto addr = D->GetEndpointByName(target);
          if (!addr.IsValid()) {
            resultCode = "No matching endpoint found.";
          }
          else {
            auto *smsg = new FSimpleMessage;
            std::string body(msgbody.data<char>(), msgbody.size());
            smsg->Message = body.c_str();
            //UE_LOG(LogTemp, Warning, TEXT("ActorMsg: target=%s message=%s"), *target, *smsg->Message);
            Endpoint->Send(smsg, addr);
            resultCode = "OK";
          }
        }

        retmsg=FString::Printf(TEXT(
          "{\n"
          " \"Type\" : \"ActorMsg\",\n"
          " \"Result\" : \"%s\"\n"
          "}"
        ), *resultCode);
      }
      else {
        retmsg=FString::Printf(TEXT(
          "{\n"
          " \"Type\" : \"%s\",\n"
          " \"Result\" : \"Unknown command\"\n"
          "}"
        ), *type);
      }
      std::string rbuf(TCHAR_TO_UTF8(*retmsg));
      D->ZCmdSocket->send(rbuf.begin(), rbuf.end());
      //UE_LOG(LogTemp, Warning, TEXT("command result: %s"), *retmsg);
    }
  }
  ++modulo;
}

void ACommActor::HandleTestMessage(const FTestMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
  UE_LOG(LogTemp,Warning,TEXT("Type: %s  Sender: %s  NumRecipients: %d  NumAnnotation: %d  Date: %s"),
    *Context->GetMessageType().ToString(),
    *Context->GetSender().ToString(),
    Context->GetRecipients().Num(),
    Context->GetAnnotations().Num(),
    *Context->GetTimeSent().ToString())
  UE_LOG(LogTemp, Warning, TEXT("Message received: %s"), *Message.Message);
}

void ACommActor::HandleRegisterMessage(const FRegisterMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context){
  UE_LOG(LogTemp, Warning, TEXT("Type: %s  Sender: %s SenderTag: %s SenderEndpoint: %s  Date: %s Metadata: %s"),
    *Context->GetMessageType().ToString(),
    *Message.Name,
    *Message.Tag,
    *Context->GetSender().ToString(),
    *Context->GetTimeSent().ToString(),
    *Message.Metadata
  )

  D->RegisterEndpoint(Message.Name, Context->GetSender(), Message.Tag, Message.Metadata);
}

void ACommActor::HandleJsonMessage(const FNamedMessage &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context) {
  if (D->SocketsReady) {
    FString s = FString::Format(
      TEXT(
        "{\n"
        "\"Report\": {\n"
        "\"Name\": \"{0}\",\n"
        "\"Time\": {1},\n"
        "\"Data\": {2}\n"
        "}\n"
        "}"),
      { Message.Name, GetWorld()->GetTimeSeconds(), Message.Message });
//    UE_LOG(LogTemp, Warning, TEXT(" JsonMessage: %s"), *s);
    std::string repbuf(TCHAR_TO_UTF8(*s));
    int res=D->ZReportSocket->send(repbuf.begin(), repbuf.end());
    if(res<0)
      UE_LOG(LogTemp, Warning, TEXT("ZReportSocket::send -> %d"), res);
  }
}
