#pragma once

#include "CoreMinimal.h"
#include "Space.h"

#include <memory>

#include "SpaceWrapper.generated.h"

UCLASS(Blueprintable)
class ASpace : public AActor
{
  GENERATED_BODY()

protected:
  std::shared_ptr<SpaceTime> Space = std::make_shared<SpaceTime>(std::numeric_limits<float>::infinity());

public:
  UFUNCTION(BlueprintCallable)
  bool IsTraversable(FPoint Point);

  UFUNCTION(BlueprintCallable)
  void InitFromFile(FString FileName);

  std::shared_ptr<SpaceTime> GetSpace() const
  {
    return Space;
  }

  UFUNCTION(BlueprintCallable)
  FVector Translate(FPoint Point) const;

  UFUNCTION(BlueprintCallable)
  FPoint Projection(FVector Location) const;

  UFUNCTION(BlueprintCallable)
  void ChangeSpaceUnsafe(FPoint Point, bool IsTraversable);
};
