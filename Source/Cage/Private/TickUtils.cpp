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

void IPostPhysicsTickable::EnablePostPhysicsTickHelper(UActorComponent *comp)
{
  auto actor = comp->GetOwner();
  if (actor) {
    PostPhysicsTickFunction.Target = comp;
    PostPhysicsTickFunction.SetTickFunctionEnable(PostPhysicsTickFunction.bStartWithTickEnabled);
    PostPhysicsTickFunction.RegisterTickFunction(actor->GetLevel());
  }
}
