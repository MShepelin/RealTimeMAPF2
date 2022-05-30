#include "Heuristic.h"

#include <algorithm>
#include <cmath>

EuclideanHeuristic::EuclideanHeuristic(FPoint InOrigin, float InSpeed)
  : Heuristic(InOrigin)
  , Origin(InOrigin)
  , Speed(InSpeed)
{

}

float EuclideanHeuristic::GetCost(FPoint To) const
{
  float DeltaX = std::abs((float)Origin.X - To.X);
  float DeltaY = std::abs((float)Origin.Y - To.Y);

  return std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY) / Speed;
}

void EuclideanHeuristic::FindCost(FPoint To)
{
  return;
}
