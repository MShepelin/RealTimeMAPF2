#pragma once

#include "Agent.h"

#include <limits>

float MakeStepInSquare(FVector2D& Point, const FVector2D& Speed, FPoint& MoveDescription);

void SetLineTimings(std::unordered_map<FPoint, Segment>& Segments, const FVector2D& Speed, FVector2D StartPoint);

std::unordered_map<FPoint, Segment> GetTouchedSegments(const MoveDelta<FPoint>& Move);
