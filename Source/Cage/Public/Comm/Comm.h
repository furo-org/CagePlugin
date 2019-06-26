// Copyright 2018 Tomoaki Yoshida<yoshida@furo.org>
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Comm/Types.h"
#include "MessageEndpointBuilder.h"
#include "MessageEndpoint.h"
#include "Templates/SharedPointer.h"

class CommEndpointOut {
public:

  ~CommEndpointOut() {
    Endpoint.Reset();
  }
  void setup(FName name) {
    Endpoint = FMessageEndpoint::Builder(name)
      .Handling<FAnnounce>(this, &CommEndpointOut::HandleAnnounce);
    if (Endpoint.IsValid()) {
      Endpoint->Subscribe<FAnnounce>();
    }
  }
  void tearDown() {
    Endpoint.Reset();
  }

  bool IsValid() { return Comm.IsValid(); }

  template<typename MessageType>
  void Send(MessageType* msg) {
    if(IsValid())
      Endpoint->Send(msg, Comm);
  }

  void HandleAnnounce(const FAnnounce &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context) {
    Comm = Context->GetSender();
  }

  TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> Endpoint;
  FMessageAddress Comm;
};

template <typename MT>
class CommEndpointIO{
  FString Tag;
  FString Name;
  FString Meta;
public:
  ~CommEndpointIO() {
    Endpoint.Reset();
  }
  void setup(FName name, FString tag, FString meta=FString()) {
    Tag = tag;
    Name = name.ToString();
    Meta = meta;
    Endpoint = FMessageEndpoint::Builder(name)
      .Handling<FAnnounce>(this, &CommEndpointIO::HandleAnnounce)
      .Handling<MT>(this, &CommEndpointIO::HandleMessage);
    if (Endpoint.IsValid()) {
      Endpoint->Subscribe<FAnnounce>();
    }
  }
  void tearDown() {
    Endpoint.Reset();
  }

  bool IsValid() { return Comm.IsValid(); }

  template<typename SMT>
  void Send(SMT* msg) {
    if (IsValid())
      Endpoint->Send(msg, Comm);
  }

  bool RecvLatest(MT *msg) {
    if (Messages.Num() == 0)return false;
    *msg = Messages[Messages.Num() - 1];
    Messages.Empty();
    return true;
  }

  void HandleAnnounce(const FAnnounce &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context) {
    if (Comm.IsValid())return;
    Comm = Context->GetSender();
    FRegisterMessage *reg=new FRegisterMessage;
    reg->Name = Name;
    reg->Tag = Tag;
    reg->Metadata = Meta;
    Endpoint->Send(reg,Comm);
  }

  void HandleMessage(const MT &Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context) {
    Comm = Context->GetSender();
    Messages.Add(Message);
  }

  TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> Endpoint;
  FMessageAddress Comm;
  TArray<MT> Messages;
};
