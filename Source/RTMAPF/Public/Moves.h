#pragma once

#include "CoreMinimal.h"
#include "SearchTypes.h"
#include "Space.h"

#include "Moves.generated.h"

template<typename CellType>
struct MoveDelta
{
  float MoveCost;
  CellType Destination;
  float WaitCost = 0;
};

USTRUCT(BlueprintType)
struct FPointMove
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere)
  float Cost;

  UPROPERTY(EditAnywhere)
  FPoint Delta;
  
  MoveDelta<FPoint> GetMoveDelta(float SpeedModifier = 1.f) const
  {
    // check(SpeedModifier > 0.f);
    return { Cost / SpeedModifier, Delta, 0 };
  }
};

template<typename CellType>
class MoveComponent
{
public:
  virtual ArrayType<MoveDelta<CellType>> FindValidMoves(const Node<CellType>& Node) = 0;

  virtual ~MoveComponent() {};
};
