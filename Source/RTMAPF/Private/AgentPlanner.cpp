#include "AgentPlanner.h"
#include "Async/Async.h"
#include "Math/UnrealMathVectorCommon.h"

#include <limits>

FAdaptivePath::FAdaptivePath(
  UAgent* InAgent, 
  std::shared_ptr<SpaceTime> InSpace, 
  float InDepth, 
  float InCurrentTime, 
  float InInactivityDelay
)
  : Agent(InAgent)
  , Space(InSpace)
  , Depth(InDepth)
  , CurrentTime(InCurrentTime)
  , InactivityDelay(InInactivityDelay)
{
  check(Depth > 0);
}

FAdaptivePath::~FAdaptivePath()
{
  if (!ReplanResult.IsReady())
  {
    ReplanResult.Wait();
  }

  ClearAreasWithPath(ReversedPath);
}

FAdaptivePath::FAdaptivePath(FAdaptivePath&& Other)
  : Agent(Other.Agent)
  , Space(Other.Space)
  , ReversedPath(Other.ReversedPath)
  , NextNodeIndex(Other.NextNodeIndex)
  , Depth(Other.Depth)
  , CurrentTime(Other.CurrentTime)
  , InactivityDelay(Other.InactivityDelay)
  , AgentShapeCapture(Other.AgentShapeCapture)
{
  Other.ReversedPath.clear();
}

const Node<Area>& FAdaptivePath::GetNextNode() const
{
  return ReversedPath.at(ReversedPath.size() - NextNodeIndex - 1);
}

const Node<Area>& FAdaptivePath::GetPreviousNode() const
{
  return ReversedPath.at(ReversedPath.size() - NextNodeIndex);
}

void FAdaptivePath::MoveTimeBy(float DeltaTime)
{
  check(DeltaTime >= 0);
  check(NextNodeIndex > 0);

  FScopeLock PathLock(&PathSync);
  CurrentTime += DeltaTime;

  while (ReversedPath.size() > NextNodeIndex && GetNextNode().MinTime < CurrentTime)
  {
    ++NextNodeIndex;
  }

  if (ReversedPath.size() <= NextNodeIndex)
  {
    if (ReversedPath.size() >= 1)
    {
      NextNodeIndex = ReversedPath.size() - 1;
    }
    else
    {
      NextNodeIndex = 1;
    }
  }
}

FPathPoint FAdaptivePath::GetNextMove(ASpace* SpaceWrapper) const
{
  check(SpaceWrapper && Agent);
  FScopeLock PathLock(&PathSync);
  if (ReversedPath.size() > NextNodeIndex)
  {
    const auto& NextNode = GetNextNode();
    const float MoveBeginTime = NextNode.MinTime - NextNode.ArrivalCost;
    if (CurrentTime < MoveBeginTime)
    {
      return { MoveBeginTime, SpaceWrapper->Translate(GetPreviousNode().Cell.Point) };
    }
    if (CurrentTime <= NextNode.MinTime)
    {
      return { NextNode.MinTime, SpaceWrapper->Translate(NextNode.Cell.Point) };
    }
    return  { CurrentTime + InactivityDelay, SpaceWrapper->Translate(NextNode.Cell.Point) };
  }
  return  { CurrentTime + InactivityDelay, SpaceWrapper->Translate(Agent->GetStartSafe()) };
}

bool FAdaptivePath::CheckForUpdate()
{
  if (!ReplanResult.IsReady())
  {
    return false;
  }

  ReplanChanges Changes = ReplanResult.Get();
  ReplanResult.Reset();
  if (!Changes.ReplanSeccess)
  {
    // Replanning failed and the path is updated with itself
    return true;
  }

  // No sync lock
  ReversedPath = std::move(Changes.ReversedPath);
  NextNodeIndex = 1;
  MoveTimeBy(0);

  return true;
}

FVector FAdaptivePath::GetCurrentLocation(ASpace* SpaceWrapper) const
{
  check(SpaceWrapper);
  FScopeLock PathLock(&PathSync);

  if (ReversedPath.size() < NextNodeIndex)
  {
    return SpaceWrapper->Translate(Agent->GetStartSafe());
  }

  const auto& NextNode = GetNextNode();
  const auto& PrevNode = GetPreviousNode();

  FVector PrevNodeLocation = SpaceWrapper->Translate(PrevNode.Cell.Point);
  FVector NextNodeLocation = SpaceWrapper->Translate(NextNode.Cell.Point);

  float MovementStartTime = NextNode.MinTime - NextNode.ArrivalCost;
  if (CurrentTime < MovementStartTime)
  {
    return PrevNodeLocation;
  }
  if (CurrentTime < NextNode.MinTime)
  {
    return FMath::Lerp(PrevNodeLocation, NextNodeLocation, (CurrentTime - MovementStartTime) / NextNode.ArrivalCost);
  }

  return NextNodeLocation;
}

struct RepairDetails
{
  Node<Area> PrevNode;
  float NextNodeArrivalCost;

  RepairDetails(const Node<Area>& InPrevNode, float InNextNodeArrivalCost)
  {
    PrevNode = InPrevNode;
    PrevNode.ArrivalCost = 0;
    PrevNode.Parent = nullptr;
    NextNodeArrivalCost = InNextNodeArrivalCost;
  }
};

bool FAdaptivePath::Replan(float InDepth)
{
  check(Agent);
  if (ReplanResult.IsValid())
  {
    return false;
  }

  // Depth is changed only when async task is empty or done
  Depth = InDepth;

  ReplanResult = Async(EAsyncExecution::ThreadPool, [this]() -> ReplanChanges {
    ReplanChanges Changes = { false };
    float AgentTimeCapture;
    
    {
      FScopeLock PathLock(&PathSync);
      AgentTimeCapture = CurrentTime;
      Changes.ReversedPath = ReversedPath;
    }

    ClearAreasWithPath(Changes.ReversedPath);

    // Gather Agent properties
    int AgentID;
    float AgentSpeed;
    FPoint AgentGoal, AgentPoint;
    std::vector<MoveDelta<FPoint>> Moves;
    Agent->GetPropertiesSafe(AgentID, AgentPoint, AgentGoal, AgentShapeCapture, Moves, AgentSpeed);
    float bestSpeed = 1.f;

    TOptional<RepairDetails> Repair;
    if (Changes.ReversedPath.size())
    {
      AgentPoint = GetPreviousNode().Cell.Point;
      const auto& NextNode = GetNextNode();
      float MovementStartTime = NextNode.MinTime - NextNode.ArrivalCost;
      if (MovementStartTime < AgentTimeCapture)
      {
        AgentTimeCapture = NextNode.MinTime;
        AgentPoint = NextNode.Cell.Point;
        Repair = RepairDetails(GetPreviousNode(), NextNode.ArrivalCost);
      }
    }

    // Prepare Agent Space and Movement Component
    std::shared_ptr<ShapeSpace> AgentSpace = std::make_shared<ShapeSpace>(std::numeric_limits<float>::infinity(), Space, AgentShapeCapture);
    std::shared_ptr<MovesTestSegment> MovesComponent(new MovesTestSegment(Moves, AgentSpace, AgentTimeCapture + Depth));
    AgentSpace->UpdateShape(AgentPoint);
    AgentSpace->UpdateShape(AgentGoal);

    if (!AgentSpace->ContainsSegmentsIn(AgentPoint))
    {
      UE_LOG(LogTemp, Warning, TEXT("Failed to init agent with id = %d (probably, initial location is occupied)"), AgentID);
      FillAreasWithPath(Changes.ReversedPath);
      return Changes;
    }

    TOptional<Area> OriginalAreaOpt = AgentSpace->FindArea(AgentPoint, AgentTimeCapture);
    if (!OriginalAreaOpt)
    {
      UE_LOG(LogTemp, Warning, TEXT("Failed to find suitable initial safe interval for an agent with id = %d"), AgentID);
      FillAreasWithPath(Changes.ReversedPath);
      return Changes;
    }

    // Prepare pathfinding
    Area OriginalArea = OriginalAreaOpt.GetValue();
    std::shared_ptr<EuclideanHeuristic> SimpleHeurisitc(new EuclideanHeuristic(AgentPoint, AgentSpeed));
    std::shared_ptr<Pathfinder<FPoint>> PlaneSearch(new Pathfinder<FPoint>(MovesComponent, AgentGoal, SimpleHeurisitc));
    std::shared_ptr<Heuristic<Area>> Adapter(new SpaceAdapter<FPoint, Area>(PlaneSearch));
    WindowedPathfinder<Area> Pathfinder(MovesComponent, OriginalArea, Adapter, AgentTimeCapture + Depth, AgentTimeCapture);

    // Execute pathfinding
    Area Destination = Area::FromDepth(AgentGoal, AgentTimeCapture + Depth);
    Pathfinder.FindCost(Destination);
    if (!Pathfinder.IsCostFound(Destination))
    {
      std::shared_ptr<OneCellHeuristic<FPoint>> OnePointHeuristic(new OneCellHeuristic<FPoint>(AgentPoint));
      std::shared_ptr<Heuristic<Area>> NewAdapter(new SpaceAdapter<FPoint, Area>(OnePointHeuristic));
      Pathfinder = WindowedPathfinder<Area>(MovesComponent, OriginalArea, NewAdapter, AgentTimeCapture + Depth, AgentTimeCapture);

      Pathfinder.FindCost(Destination);
      if (!Pathfinder.IsCostFound(Destination))
      {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find path for an agent with id = %d"), AgentID);
        FillAreasWithPath(Changes.ReversedPath);
        return Changes;
      }
    }

    // Gather Results
    Pathfinder.CollectPath(Destination, Changes.ReversedPath, true);
    Changes.ReplanSeccess = true;

    if (Repair)
    {
      Changes.ReversedPath.back().ArrivalCost = Repair.GetValue().NextNodeArrivalCost;
      Changes.ReversedPath.push_back(Repair.GetValue().PrevNode);
    }

    FillAreasWithPath(Changes.ReversedPath);
    return Changes;
  });

  return true;
}

void FAdaptivePath::ClearAreasWithPath(const std::vector<Node<Area>>& InReversedPath) const
{
  if (InReversedPath.size())
  {
    ArrayType<Area> InaccessableParts;
    FromReversedPathToFilledAreas(InReversedPath, AgentShapeCapture, InaccessableParts);
    Space->MakeAreasAccessable(InaccessableParts);
  }
}

void FAdaptivePath::FillAreasWithPath(const std::vector<Node<Area>>& InReversedPath) const
{
  if (InReversedPath.size())
  {
    ArrayType<Area> InaccessableParts;
    FromReversedPathToFilledAreas(InReversedPath, AgentShapeCapture, InaccessableParts);
    Space->MakeAreasInaccessable(InaccessableParts);
  }
}
