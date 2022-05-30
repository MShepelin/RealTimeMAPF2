#pragma once

#include "Agent.h"
#include "SearchTypes.h"
#include "Space.h"

#include <memory>

template<class CellType>
class Heuristic
{
public:
  // TODO write comments To functions and desribe how 
  // IsCostFound should be used before GetCost

  Heuristic() = delete;
  Heuristic(CellType Origin) {};

  virtual bool IsCostFound(CellType To) const { return true; };

  virtual float GetCost(CellType To) const { return 0.f; };

  virtual void FindCost(CellType To) { };

  virtual CellType GetOrigin() const { return CellType(); }

  virtual ~Heuristic() {};
};

template<class CellType>
class OneCellHeuristic : public Heuristic<FPoint>
{
private:
  CellType Origin;

public:
  OneCellHeuristic(FPoint InOrigin)
    : Heuristic<FPoint>(InOrigin)
    , Origin(InOrigin)
  {}

  virtual bool IsCostFound(CellType To) const override
  {
    return Origin == To;
  }
};

class EuclideanHeuristic : public Heuristic<FPoint>
{
private:
  FPoint Origin;
  float Speed;

public:
  EuclideanHeuristic(FPoint Origin, float Speed = 1.f);

  virtual float GetCost(FPoint To) const;

  virtual void FindCost(FPoint To);
};

template<typename FromType, typename ToType>
class SpaceAdapter : public Heuristic<ToType>
{
  std::shared_ptr<Heuristic<FromType>> HeuristicPtr;

public:
  SpaceAdapter(std::shared_ptr<Heuristic<FromType>> InHeuristic)
    : Heuristic<ToType>(ToType(InHeuristic->GetOrigin()))
    , HeuristicPtr(InHeuristic)
  {}

  virtual bool IsCostFound(ToType To) const override { return HeuristicPtr->IsCostFound(FromType(To)); }

  virtual float GetCost(ToType To) const override { return HeuristicPtr->GetCost(FromType(To)); }

  virtual void FindCost(ToType To) override { return HeuristicPtr->FindCost(FromType(To)); }

  virtual ToType GetOrigin() const override { return ToType(HeuristicPtr->GetOrigin()); }
};
