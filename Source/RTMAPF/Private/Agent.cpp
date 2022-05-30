#include "Agent.h"
#include "MAPF.h"
#include "Segments.h"
#include "MovesSegments.h"
#include "Kismet/GameplayStatics.h"

ArrayType<MoveDelta<Area>> MovesTestSegment::FindValidMoves(const Node<Area>& Node)
{
  ArrayType<MoveDelta<Area>> Result;
  Area Origin = Node.Cell;

  Segment MoveAvailable{ Node.MinTime, Node.Cell.Interval.End };
  if (Node.Cell.Interval.End >= Depth)
  {
    // Fictive node
    Result.push_back({ 0, Area{Origin.Point, {Depth, Node.Cell.Interval.End }}, Depth - Node.MinTime });
  }

  for (MoveDelta<FPoint> Move : Moves)
  {
    FPoint DestinationPoint = Origin.Point + Move.Destination;
    Space->UpdateShape(DestinationPoint);
    if (!Space->ContainsSegmentsIn(DestinationPoint)) continue;

    const auto RelationalPointToSegment = GetTouchedSegments(Move);
    auto OriginMoveSegment = RelationalPointToSegment.at({ 0, 0 });
    auto DestinationMoveSegment = RelationalPointToSegment.at(Move.Destination);
    
    SegmentHolder DestinationSegmentHolder = MoveAvailable;
    DestinationSegmentHolder -= OriginMoveSegment.Start;
    DestinationSegmentHolder.LowerSegments(OriginMoveSegment.GetLength());

    for (const auto& PointAndSegment : RelationalPointToSegment)
    {
      const auto MovePoint = Origin.Point + PointAndSegment.first;
      const auto MovementSegment = PointAndSegment.second;

      Space->UpdateShape(MovePoint);
      if (!Space->ContainsSegmentsIn(MovePoint))
      {
        // Impossible Move
        DestinationSegmentHolder = SegmentHolder();
        break;
      }

      SegmentHolder MovePointHolder = Space->GetSegments(MovePoint);
      MovePointHolder -= MovementSegment.Start;
      MovePointHolder.LowerSegments(MovementSegment.GetLength());
      DestinationSegmentHolder = DestinationSegmentHolder & MovePointHolder;
    }

    const auto OriginalDestinationSegments = Space->GetSegments(DestinationPoint);

    std::unordered_map<Segment, float> DestinationSegmentToMinTime;
    for (auto& DestinationSegment : DestinationSegmentHolder)
    {
      const float TimeOnDestination = DestinationSegment.Start + DestinationMoveSegment.Start;
      Segment OriginalSegment = OriginalDestinationSegments.Find(
        TimeOnDestination
      );

      if (OriginalSegment.IsValid())
      {
        // check(TimeOnDestination <= OriginalSegment.end && TimeOnDestination >= OriginalSegment.start - EPSILON);
        if (!DestinationSegmentToMinTime.count(OriginalSegment))
        {
          DestinationSegmentToMinTime[OriginalSegment] = DestinationSegment.Start + OriginMoveSegment.Start;
        }
      }
    }

    for (auto& SegmentAndTime : DestinationSegmentToMinTime)
    {
      const float MovementStartTime = SegmentAndTime.second;
      const float MovementEndTime = SegmentAndTime.second + DestinationMoveSegment.GetLength();
      check(MovementStartTime >= Node.MinTime);
      Result.push_back({ Move.MoveCost, Area{DestinationPoint, SegmentAndTime.first}, MovementStartTime - Node.MinTime });
    }
  }

  return Result;
}

ArrayType<MoveDelta<FPoint>> MovesTestSegment::FindValidMoves(const Node<FPoint>& Node)
{
  ArrayType<MoveDelta<FPoint>> Result;
  FPoint Origin = Node.Cell;

  for (MoveDelta<FPoint> Move : Moves)
  {
    bool Error = false;
    const auto RelationalPointToSegment = GetTouchedSegments(Move);
    for (const auto& PointAndSegment : RelationalPointToSegment)
    {
      auto MovePoint = Origin + PointAndSegment.first;
      Space->UpdateShape(MovePoint);
      if (!Space->ContainsSegmentsIn(MovePoint))
      {
        Error = true;
        break;
      }
      const auto& Segments = Space->GetSegments(MovePoint);
      if (Segments.begin() == Segments.end())
      {
        Error = true;
        break;
      }
    }
    
    if (!Error)
    {
      Result.push_back({ Move.MoveCost, Origin + Move.Destination });
    }
  }

  return Result;
}

void UAgent::Disconnect()
{
  if (!bIsConnected || !GetWorld()) return;

  UGameInstance* GameInstance = GetWorld()->GetGameInstance();
  check(GameInstance);
  UMultiagentPathfinder* MAPFSubsystem = GameInstance->GetSubsystem<UMultiagentPathfinder>();
  check(MAPFSubsystem);

  UE_LOG(LogTemp, Log, TEXT("Removing agent with ID=%d"), AgentID);
  MAPFSubsystem->RemoveAgent(AgentID);

  bIsConnected = false;
}

void UAgent::BeginDestroy()
{
  Super::BeginDestroy();
  Disconnect();
}

UMultiagentPathfinder* UAgent::GetMAPF() const
{
  UGameInstance* GameInstance = GetWorld()->GetGameInstance();
  UMultiagentPathfinder* MAPFSubsystem = GameInstance->GetSubsystem<UMultiagentPathfinder>();
  if (!MAPFSubsystem)
  {
    UE_LOG(LogTemp, Error, TEXT("MAPF Subsystem not found"));
    return nullptr;
  }

  return MAPFSubsystem;
}

void UAgent::InitFrom(FAgentTask Task)
{
  Start = Task.Start;
  Goal = Task.Goal;

  auto MAPFSubsystem = GetMAPF();
  if (!MAPFSubsystem) return;

  UE_LOG(LogTemp, Verbose, TEXT("UAgent: Adding new agent"));
  MAPFSubsystem->AddAgent(this);

  // TODO log error on second init 
}

FVector UAgent::GetCurrentLocation() const
{
  if (!bIsConnected)
  {
    UE_LOG(LogTemp, Error, TEXT("Calling GetCurrentLocation before connection to MAPF subsystem"));
    return {};
  }

  auto MAPFSubsystem = GetMAPF();
  if (!MAPFSubsystem) return {};

  return MAPFSubsystem->GetCurrentLocation(AgentID);
}

FPathPoint UAgent::GetNextMove() const
{
  if (!bIsConnected)
  {
    UE_LOG(LogTemp, Error, TEXT("Calling GetNextMove before connection to MAPF subsystem"));
    return {};
  }

  auto MAPFSubsystem = GetMAPF();
  if (!MAPFSubsystem) return {};

  return MAPFSubsystem->GetNextMove(AgentID);
}

void UAgent::ChangeGoal(FPoint NewGoal)
{
  auto MAPFSubsystem = GetMAPF();
  if (!MAPFSubsystem) return;

  FScopeLock g(&PropertiesSync);

  Goal = NewGoal;
  // Start is not updated as it is used only to init agent

  MAPFSubsystem->ForceReplan(AgentID);
}

void UAgent::GetPropertiesSafe(
  int& OuterID,
  FPoint& OuterStart,
  FPoint& OuterGoal,
  FShape& OuterShape,
  ArrayType<MoveDelta<FPoint>>& OuterMoves,
  float& OuterSpeed
) const {
  FScopeLock g(&PropertiesSync);

  OuterID = AgentID;
  OuterStart = Start;
  OuterGoal = Goal;
  OuterShape = Shape;
  OuterSpeed = SpeedModifier;

  for (const auto& Move : Moves)
  {
    OuterMoves.push_back(Move.GetMoveDelta(SpeedModifier));
  }
}
