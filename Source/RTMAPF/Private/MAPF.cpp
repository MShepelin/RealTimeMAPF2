#include "MAPF.h"

UMultiagentPathfinder::UMultiagentPathfinder()
{
  Depth = 30;
}

void UMultiagentPathfinder::SetDepth(float InDepth)
{
  Depth = InDepth;
  // TODO add depth setting in AdaptivePath
}

void UMultiagentPathfinder::Initialize(FSubsystemCollectionBase& Collection)
{

}

void UMultiagentPathfinder::Deinitialize()
{
  Reset();
}

void UMultiagentPathfinder::SetSpace(ASpace* InSpaceWrapper)
{
  check(InSpaceWrapper);
  SpaceWrapper = InSpaceWrapper;
  Space = InSpaceWrapper->GetSpace();
}

void UMultiagentPathfinder::Reset()
{
  FScopeLock g(&AccessAgentPaths);
  AgentPaths.Empty();
  CurrentlyReplanning.Reset();
  Space = nullptr;
  SpaceWrapper = nullptr; 
  PendingRemove = false;
  ReplanningFreshAgent = false;
  ReplanRepeat = false;
}

void UMultiagentPathfinder::Tick(float DeltaTime)
{
  FScopeLock g(&AccessAgentPaths);
  check(SpaceWrapper);
  check(DeltaTime >= 0);

  CurrentTime += DeltaTime;
  for (auto& AdaptivePath : AgentPaths)
  {
    AdaptivePath.Value.MoveTimeBy(DeltaTime);
  }

  // Check if we need to replan
  if (CurrentlyReplanning)
  {
    const auto& AdaptivePath = AgentPaths.Find(CurrentlyReplanning.GetValue());
    if (!AdaptivePath->CheckForUpdate())
    {
      // Still planning
      return;
    }

    if (ReplanningFreshAgent)
    {
      if (!AdaptivePath->IsAnyPathReady())
      {
        if (!PendingRemove)
        {
          AdaptivePath->GetAgent()->ConnectionFailed();
        }
        PendingRemove = true;
        UE_LOG(LogTemp, Error, TEXT("New agent with id = %d failed to enter MAPF subsystem"), AdaptivePath->GetAgent()->GetIDUnsafe());
      }
      else
      {
        if (!PendingRemove)
        {
          AdaptivePath->GetAgent()->MarkConnection();
        }
      }

      ReplanningFreshAgent = false;
    }

    if (PendingRemove)
    {
      AgentPaths.Remove(CurrentlyReplanning.GetValue());
      PendingRemove = false;
    }
    else if (ReplanRepeat)
    {
      ReplanRepeat = false;
      bool ReplanBegin = AgentPaths[CurrentlyReplanning.GetValue()].Replan(Depth);
      check(ReplanBegin);
      return;
    }
    else
    {
      AgentPaths[CurrentlyReplanning.GetValue()].GetAgent()->OnReplan.Broadcast();
      Order.AddTail(CurrentlyReplanning.GetValue());
    }

    CurrentlyReplanning.Reset();
  }

  // Change CurrentlyReplanning
  if (PendingToAdd.Num())
  {
    UAgent* Agent = PendingToAdd.Pop();
    while (AgentPaths.Contains(Agent->GetIDUnsafe()))
    {
      // TODO make something smarter
      Agent->SetIDUnsafe(MaxAgentID++);
    }

    AgentPaths.Add(Agent->GetIDUnsafe(), FAdaptivePath(Agent, Space, Depth, CurrentTime));
    ReplanningFreshAgent = true;
    CurrentlyReplanning = Agent->GetIDUnsafe();
    bool ReplanBegin = AgentPaths[Agent->GetIDUnsafe()].Replan(Depth);
    check(ReplanBegin);
    return;
  }

  while (Order.Num() && !AgentPaths.Contains(Order.GetHead()->GetValue()))
  {
    Order.RemoveNode(Order.GetHead());
  }

  if (Order.Num())
  {
    CurrentlyReplanning = AgentPaths[Order.GetHead()->GetValue()].GetAgent()->GetIDUnsafe();
    bool ReplanBegin = AgentPaths[CurrentlyReplanning.GetValue()].Replan(Depth);
    check(ReplanBegin);
    Order.RemoveNode(Order.GetHead());
  }
}

FVector UMultiagentPathfinder::GetCurrentLocation(int ID) const
{
  FScopeLock g(&AccessAgentPaths);
  check(AgentPaths.Contains(ID));
  return AgentPaths.Find(ID)->GetCurrentLocation(SpaceWrapper);
}

FPathPoint UMultiagentPathfinder::GetNextMove(int ID) const
{
  FScopeLock g(&AccessAgentPaths);
  check(AgentPaths.Contains(ID));
  return AgentPaths.Find(ID)->GetNextMove(SpaceWrapper);
}

float UMultiagentPathfinder::GetCurrentTime() const
{
  return CurrentTime;
}

void UMultiagentPathfinder::AddAgent(UAgent* Agent)
{
  FScopeLock g(&AccessAgentPaths);
  check(Space);
  PendingToAdd.Add(Agent);
}

void UMultiagentPathfinder::RemoveAgent(int ID)
{
  FScopeLock g(&AccessAgentPaths);
  check(Space);
  if (!AgentPaths.Contains(ID))
  {
    UE_LOG(LogTemp, Error, TEXT("Attempted to remove nonexisting agent"));
    return;
  }

  if (CurrentlyReplanning)
  {
    const auto& AdaptivePath = AgentPaths.Find(CurrentlyReplanning.GetValue());
    check(AdaptivePath->GetAgent());
    if (AdaptivePath->GetAgent()->GetIDUnsafe() == ID)
    {
      UE_LOG(LogTemp, Log, TEXT("Removing agent is delayed as it is planning now"));
      PendingRemove = true;
      return;
    }
  }

  AgentPaths.Remove(ID);
}

void UMultiagentPathfinder::ForceReplan(int ID)
{
  if (!AgentPaths.Contains(ID))
  {
    UE_LOG(LogTemp, Error, TEXT("Attempted to force replan with nonexisting agent"));
    return;
  }

  if (CurrentlyReplanning && CurrentlyReplanning.GetValue() == ID)
  {
    ReplanRepeat = true;
    return;
  }

  auto findNode = Order.FindNode(ID);
  if (!findNode)
  {
    UE_LOG(LogTemp, Error, TEXT("Attempted to force replan with agent that is outside of Order"));
    return;
  }

  Order.RemoveNode(findNode);
  Order.AddHead(ID);
}
