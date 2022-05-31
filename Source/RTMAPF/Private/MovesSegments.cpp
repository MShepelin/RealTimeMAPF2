#include "MovesSegments.h"

float MakeStepInSquare(FVector2D& Point, const FVector2D& Speed, FPoint& MoveDescription)
{
  float TimeToReachRight = (Speed.X > 0) ? ((1.f - Point.X) / Speed.X) : std::numeric_limits<float>::infinity();
  float TimeToReachUp = (Speed.Y > 0) ? ((1.f - Point.Y) / Speed.Y) : std::numeric_limits<float>::infinity();

  if (std::abs(TimeToReachUp - TimeToReachRight) < EPSILON)
  {
    Point.X = 0;
    Point.Y = 0;
    MoveDescription = { 1, 1 };
    return TimeToReachUp;
  }

  if (TimeToReachUp > TimeToReachRight)
  {
    Point.X = 0.f;
    Point.Y += TimeToReachRight * Speed.Y;
    MoveDescription = { 1, 0 };
    return TimeToReachRight;
  }
  else
  {
    Point.X += TimeToReachUp * Speed.X;
    Point.Y = 0.f;
    MoveDescription = { 0, 1 };
    return TimeToReachUp;
  }
}

void SetLineTimings(std::unordered_map<FPoint, Segment>& Segments, const FVector2D& Speed, FVector2D StartPoint)
{
  float CurrentTime = 0.f;

  FPoint MoveDescription = { 0, 0 };
  FPoint Point = { 0, 0 };
  while (1.f > CurrentTime)
  {
    Segment NewSegment = { CurrentTime, CurrentTime };
    CurrentTime += MakeStepInSquare(StartPoint, Speed, MoveDescription);
    NewSegment.End = std::min(CurrentTime, 1.f);

    Segments[Point] = (Segments.count(Point) > 0) ? (Segments[Point] | NewSegment) : NewSegment;

    Point = Point + MoveDescription;
  }
}

std::unordered_map<FPoint, Segment> GetTouchedSegments(const MoveDelta<FPoint>& Move)
{
  static std::unordered_map<FPoint, std::unordered_map<FPoint, Segment>> PointToSegments;
  std::unordered_map<FPoint, Segment> Result;
  if (PointToSegments.count(Move.Destination) > 0)
  {
    Result = PointToSegments[Move.Destination];
  }
  else
  {
    FPoint Direction = { 1, 1 };
    FPoint PositiveDestination = Move.Destination;
    if (PositiveDestination.X < 0)
    {
      PositiveDestination.X = -PositiveDestination.X;
      Direction.X = -1;
    }
    if (PositiveDestination.Y < 0)
    {
      PositiveDestination.Y = -PositiveDestination.Y;
      Direction.Y = -1;
    }

    const FVector2D Speed = { (float) PositiveDestination.X, (float) PositiveDestination.Y };
    FVector2D ShapeDelta = Speed;
    ShapeDelta.Normalize();

    const FVector2D StartPoint = { 0.5, 0.5 };
    std::unordered_map<FPoint, Segment> PositiveResult;
    SetLineTimings(PositiveResult, Speed, StartPoint - ShapeDelta / 2);
    SetLineTimings(PositiveResult, Speed, StartPoint + ShapeDelta / 2);
    SetLineTimings(PositiveResult, Speed, StartPoint + FVector2D(ShapeDelta.X, -ShapeDelta.Y) / 2);
    SetLineTimings(PositiveResult, Speed, StartPoint + FVector2D(-ShapeDelta.X, ShapeDelta.Y) / 2);

    for (const auto& ResultPair : PositiveResult)
    {
      Result[ResultPair.first * Direction] = { 
        ResultPair.second.Start, 
        ResultPair.second.End
      };
    }

    PointToSegments[Move.Destination] = Result;
  }

  for (const auto& ResultPair : Result)
  {
    Result[ResultPair.first] = {
      ResultPair.second.Start * Move.MoveCost,
      ResultPair.second.End * Move.MoveCost
    };
  }

  return Result;
}
