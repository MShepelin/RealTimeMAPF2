#include "SpaceWrapper.h"
#include "Kismet/KismetMathLibrary.h"

#include <fstream>

bool ASpace::IsTraversable(FPoint Point)
{
  if (!Space)
  {
    UE_LOG(LogTemp, Error, TEXT("Tried to use space wrapper, but space wasn't initialised"));
    return false;
  }

  return Space->ContainsSegmentsIn(Point);
}

void ASpace::InitFromFile(FString FileName)
{
  std::ifstream SpaceFile(*FileName);
  if (!SpaceFile.is_open())
  {
    UE_LOG(LogTemp, Error, TEXT("Cannot open File %s"), *FileName);
    return;
  }

  SpaceReader Reader;
  TOptional<RawSpace> RawSpace = Reader.FromHogFormat(SpaceFile);
  if (!RawSpace)
  {
    UE_LOG(LogTemp, Error, TEXT("Cannot read hog format from %s"), *FileName);
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("File %s processed correctly"), *FileName);

  Space = std::make_shared<SpaceTime>(std::numeric_limits<float>::infinity(), RawSpace.GetValue());
}

FVector ASpace::Translate(FPoint Point) const
{
  const FVector Scale = GetActorScale();
  FVector X, Y, Z;

  UKismetMathLibrary::BreakRotIntoAxes(GetActorRotation(), X, Y, Z);

  auto Location = GetActorLocation() + X * Scale.X * Point.X + Y * Scale.Y * Point.Y;

  return Location;
}

FPoint ASpace::Projection(FVector Location) const
{
  const FVector Scale = GetActorScale();
  FVector X, Y, Z;
  UKismetMathLibrary::BreakRotIntoAxes(GetActorRotation(), X, Y, Z);

  const auto PlaneDelta = UKismetMathLibrary::ProjectPointOnToPlane(Location, GetActorLocation(), Z) - GetActorLocation();
  const auto XValue = PlaneDelta.ProjectOnTo(X).Size() / Scale.X;
  const auto YValue = PlaneDelta.ProjectOnTo(Y).Size() / Scale.Y;
  
  return FPoint(static_cast<int>(XValue), static_cast<int>(YValue));
}

void ASpace::ChangeSpaceUnsafe(FPoint Point, bool IsTraversable)
{
  const auto inf = std::numeric_limits<float>::infinity();
  Space->SetAccess(Point, IsTraversable ? Access::Accessable : Access::Inaccessable, inf);
}
