#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types.h"
#include "MessageEndpoint.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "TickUtils.generated.h"

/*
   IPostPhysicsTickable: Interface for ActorComponents to receive PostPhysicsTick()

   example usage:
   class UPPComponent: public UActorComponent, public IPostPhysicsTickable{
     public:
       virtual void PostInitProperties() override{
         Super::PostInitProperties();
         if (!IsTemplate() && PostPhysicsTickFunction.bCanEverTick) {
           IPostPhysicsTickable::EnablePostPhysicsTick(this);
         }
       }
       virtual void PostPhysicsTick(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override{
          UE_LOG(LogTemp, Warning, TEXT("PostPhysicsTick"));
       }
   };
*/


USTRUCT()
struct FPostPhysicsTick : public FActorComponentTickFunction {
  GENERATED_USTRUCT_BODY()
  class UActorComponent *Target=nullptr;
  class IPostPhysicsTickable *TargetPP=nullptr;
  virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
  virtual FString DiagnosticMessage() override
  {
    return Target->GetFullName() + TEXT("[PostPhysicsTick]");
  }
  void init() {
    TickGroup = TG_PostPhysics;
    bCanEverTick = true;
    bStartWithTickEnabled = true;
  }
};
template <>
struct TStructOpsTypeTraits<FPostPhysicsTick> : public TStructOpsTypeTraitsBase2<FPostPhysicsTick>
{
  enum { WithCopy = false };
};

USTRUCT()
struct FPrePhysicsTick : public FActorComponentTickFunction {
  GENERATED_USTRUCT_BODY()
  class UActorComponent *Target=nullptr;
  class IPrePhysicsTickable *TargetPP=nullptr;
  virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
  virtual FString DiagnosticMessage() override
  {
    return Target->GetFullName() + TEXT("[PrePhysicsTick]");
  }
  void init() {
    TickGroup = TG_PrePhysics;
    bCanEverTick = true;
    bStartWithTickEnabled = true;
  }
};
template <>
struct TStructOpsTypeTraits<FPrePhysicsTick> : public TStructOpsTypeTraitsBase2<FPrePhysicsTick>
{
  enum { WithCopy = false };
};

//---------------------------------------------------------------------------

UINTERFACE()
class UPostPhysicsTickable : public UInterface {
  GENERATED_BODY()
public:
};

class IPostPhysicsTickable {
  GENERATED_BODY()
protected:
  FPostPhysicsTick PostPhysicsTickFunction;
public:
  virtual void PostPhysicsTick(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)=0;
  IPostPhysicsTickable() { PostPhysicsTickFunction.init(); }
  void EnablePostPhysicsTick(UActorComponent *comp);
};


UINTERFACE()
class UPrePhysicsTickable : public UInterface {
  GENERATED_BODY()
public:
};

class IPrePhysicsTickable {
  GENERATED_BODY()
protected:
  FPrePhysicsTick PrePhysicsTickFunction;
public:
  virtual void PrePhysicsTick(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)=0;
  IPrePhysicsTickable() { PrePhysicsTickFunction.init(); }
  void EnablePrePhysicsTick(UActorComponent *comp);
};
