#include "Shapes.h"
#include "MovesSegments.h"

ArrayType<FPoint> FShape::ApplyShapeTo(FPoint Point) const
{
  ArrayType<FPoint> Result;
  for (const auto& ShapePoint : Points)
  {
    Result.push_back(ShapePoint + Point);
  }

  return Result;
}

ShapeSpace::ShapeSpace(float Depth, std::shared_ptr<SegmentSpace> InSpace, const FShape& InShape)
  : SpaceTime(Depth)
  , OriginalSpace(InSpace)
  , Shape(InShape)
{ }

void ShapeSpace::UpdateShape(FPoint Point)
{
  if (PointCache.count(Point))
  {
    return;
  }

  PointCache.insert(Point);

  ArrayType<FPoint> JoinedPoints = Shape.ApplyShapeTo(Point);

  bool Contains = true;
  for (FPoint& OriginalSpacePoint : JoinedPoints)
  {
    if (!OriginalSpace->ContainsSegmentsIn(OriginalSpacePoint))
    {
      Contains = false;
      break;
    }
  }

  if (!Contains)
  {
    return;
  }

  SegmentGrid[Point] = SegmentHolder(Segment{ 0, Depth });
  for (FPoint& OriginalSpacePoint : JoinedPoints)
  {
    const SegmentHolder& Segments = OriginalSpace->GetSegments(OriginalSpacePoint);
    SegmentGrid[Point] = SegmentGrid[Point] & Segments;
  }
}

void FromReversedPathToFilledAreas(const ArrayType<Node<Area>>& Path, const FShape& Shape, ArrayType<Area>& Areas)
{
  const auto& LastNode = Path.front();
  Segment LastMovementOnPlace{ LastNode.MinTime, LastNode.Cell.Interval.End };
  for (const FPoint& ShapePoint : Shape.Points)
  {
    Areas.push_back(Area(LastNode.Cell.Point + ShapePoint, LastMovementOnPlace));
  }

  for (size_t CellIndex = 1; CellIndex < Path.size(); ++CellIndex)
  {
    const auto& Prev = Path[CellIndex];
    const auto& Next = Path[CellIndex - 1];

    const FPoint Delta = Next.Cell.Point - Prev.Cell.Point;
    const auto RelationalPointToSegment = GetTouchedSegments({ Next.ArrivalCost, Delta });
    const float MovementStartTime = Next.MinTime - Next.ArrivalCost;

    if (MovementStartTime > Prev.MinTime - EPSILON)
    {
      const Segment MoveOnPlace = { Prev.MinTime, MovementStartTime };
      for (const FPoint& ShapePoint : Shape.Points)
      {
        Areas.push_back(Area(Prev.Cell.Point + ShapePoint, MoveOnPlace));
      }
    }

    for (const auto& PointAndSegment : RelationalPointToSegment)
    {
      const auto MovePoint = Prev.Cell.Point + PointAndSegment.first;
      const Segment MovementSegment = { PointAndSegment.second.Start + MovementStartTime, PointAndSegment.second.End + MovementStartTime };

      for (const FPoint& ShapePoint : Shape.Points)
      {
        Areas.push_back(Area(MovePoint + ShapePoint, MovementSegment));
      }
    }
  }
}
