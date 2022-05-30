#pragma once

#include "CoreMinimal.h"
#include "SearchTypes.h"
#include "Space.h"

#include <memory>
#include <unordered_set>

#include "Shapes.generated.h"

USTRUCT(BlueprintType)
struct FShape
{
  GENERATED_BODY()

  UPROPERTY(EditDefaultsOnly)
  TArray<FPoint> Points = {
    FPoint(0, -1),
    FPoint(-1, 0),
    FPoint(0, 0),
    FPoint(1, 0),
    FPoint(0, 1)
  };

  ArrayType<FPoint> ApplyShapeTo(FPoint Point) const;
};

class ShapeSpace : public SpaceTime
{
private:
  std::shared_ptr<SegmentSpace> OriginalSpace;
  FShape Shape;

  std::unordered_set<FPoint> PointCache;

public:
  ShapeSpace() = delete;
  ShapeSpace(float Depth, const RawSpace& Base) = delete;
  ShapeSpace(float Depth, std::shared_ptr<SegmentSpace> InSpace, const FShape& InShape);

  void UpdateShape(FPoint Point);
};

void FromReversedPathToFilledAreas(const ArrayType<Node<Area>>& Path, const FShape& Shape, ArrayType<Area>& Areas);
