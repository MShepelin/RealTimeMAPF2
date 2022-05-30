#pragma once

#include "Heuristic.h"
#include "Moves.h"
#include "NodesHeap.h"
#include "SearchTypes.h"

#include <cassert>
#include <chrono>
#include <iostream>

template<typename CellType>
class SearchResult
{
private:
  std::chrono::steady_clock::time_point TimerStart = std::chrono::high_resolution_clock::now();

  double Time = 0;
  size_t NodesCreated = 0;
  size_t NumberOfSteps = 0;

public:
  inline void IncrementSteps()
  {
    NumberOfSteps++;
  }

  inline void SetNodesCount(size_t InNodesCount)
  {
    NodesCreated = InNodesCount;
  }

  inline void StartTimer()
  {
    TimerStart = std::chrono::high_resolution_clock::now();
  }

  inline void StopTimer()
  {
    // TODO create timer object which incapsulates duration count like shared pointer

    std::chrono::duration<double> Duration = std::chrono::high_resolution_clock::now() - TimerStart;
    Time += Duration.count(); // in seconds
  }
};

template<typename CellType>
class Pathfinder : public Heuristic<CellType>
{
protected:
  using NodeType = Node<CellType>;
  using StatType = SearchResult<CellType>;

  mutable StatType Statistics;

  NodesBinaryHeap<CellType> OpenNodes;
  MapType<CellType, NodeType> Nodes;

  std::shared_ptr<Heuristic<CellType>> HeuristicPtr;
  std::shared_ptr<MoveComponent<CellType>> Moves;

  virtual void TryToStopSearch(const NodeType& Node, CellType SearchDestination) {};

protected:
  void ExpandNode(NodeType& Node);

public:
  Pathfinder(
    std::shared_ptr<MoveComponent<CellType>> InMoves, 
    CellType Origin,
    std::shared_ptr<Heuristic<CellType>> InHeuristic,
    float StartTime = 0.f
  );

  virtual bool IsCostFound(CellType To) const override;
  virtual float GetCost(CellType To) const override;
  virtual void FindCost(CellType To) override;

  StatType GetStats() const { return Statistics; }

  void CollectPath(CellType To, ArrayType<NodeType>& Path, bool Reverse = false) const;

  void SetHeuristic(std::shared_ptr<Heuristic<CellType>> InHeuristic);
};

template<typename CellType>
class WindowedPathfinder : public Pathfinder<CellType>
{
protected:
  float Depth;

protected:
  virtual void TryToStopSearch(const NodeType& Node, CellType SearchDestination) override
  {
    if (Node.MinTime >= Depth)
    {
      Nodes[SearchDestination] = Node;
    }
  }

public:
  WindowedPathfinder(
     std::shared_ptr<MoveComponent<CellType>> InMoves
    , CellType Origin
    , std::shared_ptr<Heuristic<CellType>> InHeuristic
    , float InDepth
    , float StartTime
  )
    : Pathfinder(InMoves, Origin, InHeuristic, StartTime)
    , Depth(InDepth)
  {
    assert(Depth > 0);
  }
};

template<typename CellType>
Pathfinder<CellType>::Pathfinder(
  std::shared_ptr<MoveComponent<CellType>> InMoves, 
  CellType Origin,
  std::shared_ptr<Heuristic<CellType>> InHeuristic,
  float StartTime
)
  : Heuristic(Origin)
  , OpenNodes(true)
  , HeuristicPtr(InHeuristic)
  , Moves(InMoves)
{
  HeuristicPtr->FindCost(Origin);
  if (HeuristicPtr->IsCostFound(Origin))
  {
    Nodes[Origin] = Node<CellType>(Origin, StartTime, HeuristicPtr->GetCost(Origin));
    OpenNodes.Insert(Nodes[Origin]);
  }
}

template<typename CellType>
void Pathfinder<CellType>::ExpandNode(NodeType& Node)
{
  for (auto& ValidMove : Moves->FindValidMoves(Node))
  {
    const CellType& Destination = ValidMove.Destination;

    // Check if a potential Node exists
    auto PotentialNode = Nodes.find(Destination);
    if (PotentialNode == Nodes.end())
    {
      HeuristicPtr->FindCost(Destination);
      if (!HeuristicPtr->IsCostFound(Destination))
      {
        continue;
      }

      // Create a new Node.
      auto InsertResult = Nodes.insert({ 
          Destination
        , NodeType(
            Destination
          , Node.MinTime + ValidMove.WaitCost + ValidMove.MoveCost
          , HeuristicPtr->GetCost(Destination)
          , ValidMove.MoveCost
        )
      });

      NodeType& InsertedNode = InsertResult.first->second;
      OpenNodes.Insert(InsertedNode);

      // Set the parential Node.
      InsertedNode.Parent = &Node;
    }
    else
    {
      float NodeMinTime = Node.MinTime + ValidMove.WaitCost + ValidMove.MoveCost;
      if (PotentialNode->second.HeursticToGoal >= 0 && PotentialNode->second.MinTime > NodeMinTime)
      {
        OpenNodes.ImproveTime(PotentialNode->second, NodeMinTime);

        // Change the parential Node to the one which is expanded.
        PotentialNode->second.Parent = &Node;
      }
      // If the potential Node is in the close list, we never reopen/reexpand it.
    }
  }
}

template<typename CellType>
float Pathfinder<CellType>::GetCost(CellType To) const
{
  assert(IsCostFound(To));

  return Nodes.at(To).MinTime;
}
 
template<typename CellType>
bool Pathfinder<CellType>::IsCostFound(CellType To) const
{
  return Nodes.count(To) > 0;
}

template<typename CellType>
void Pathfinder<CellType>::FindCost(CellType To)
{
  Statistics.StartTimer();

  while (!IsCostFound(To) && OpenNodes.Size())
  {
    Statistics.IncrementSteps();

    NodeType& ExpandedNode = *OpenNodes.PopMin();
    ExpandedNode.MarkClosed();

    ExpandNode(ExpandedNode);
    TryToStopSearch(ExpandedNode, To);
  }

  Statistics.SetNodesCount(Nodes.size());
  Statistics.StopTimer();
}

template<typename CellType>
void Pathfinder<CellType>::CollectPath(CellType To, ArrayType<NodeType>& Path, bool Reverse) const
{
  Path.clear();

  if (!IsCostFound(To))
  {
    return;
  }

  Statistics.StartTimer();

  const NodeType* CurrentNode = &Nodes.at(To);
  while (CurrentNode)
  {
    Path.push_back(NodeType(
        CurrentNode->Cell
      , CurrentNode->MinTime
      , CurrentNode->HeursticToGoal
      , CurrentNode->ArrivalCost
    ));
    CurrentNode = CurrentNode->Parent;
  }

  if (!Reverse)
  {
    std::reverse(Path.begin(), Path.end());
  }

  Statistics.StopTimer();
}
