#include "TickUtils.h"


void FPostPhysicsTick::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
  ExecuteTickHelper(Target, Target->bTickInEditor, DeltaTime, TickType, [this, TickType](float DilatedTime) {
    if (!TargetPP) {
      TargetPP = CastChecked<IPostPhysicsTickable>(Target);
    }
    if (TargetPP) {
        TargetPP->PostPhysicsTick(DilatedTime, TickType, this);
      }
    });
}

void FPrePhysicsTick::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
  ExecuteTickHelper(Target, Target->bTickInEditor, DeltaTime, TickType, [this, TickType](float DilatedTime) {
    if (!TargetPP) {
      TargetPP = CastChecked<IPrePhysicsTickable>(Target);
    }
    if (TargetPP) {
        TargetPP->PrePhysicsTick(DilatedTime, TickType, this);
      }
    });
}

void IPostPhysicsTickable::EnablePostPhysicsTick(UActorComponent *comp)
{
  auto actor = comp->GetOwner();
  if (actor) {
    PostPhysicsTickFunction.Target = comp;
    PostPhysicsTickFunction.SetTickFunctionEnable(PostPhysicsTickFunction.bStartWithTickEnabled);
    PostPhysicsTickFunction.RegisterTickFunction(actor->GetLevel());
  }
}

void IPrePhysicsTickable::EnablePrePhysicsTick(UActorComponent *comp)
{
  auto actor = comp->GetOwner();
  if (actor) {
    PrePhysicsTickFunction.Target = comp;
    PrePhysicsTickFunction.SetTickFunctionEnable(PrePhysicsTickFunction.bStartWithTickEnabled);
    PrePhysicsTickFunction.RegisterTickFunction(actor->GetLevel());
  }
}
