#pragma once

#include "SearchTypes.h"

#include <cassert>

#define HEAP_START_CAPACITY 16

/**
 * Min heap for nodes.
 */
template<typename CellType>
class NodesBinaryHeap
{
public:
  using NodeType = Node<CellType>;

protected:
  // TODO not NodeType* but size_t, node can be moved in dynamic memory 
  // so we need to store ID, not pointer
  ArrayType<NodeType*> Nodes;

  // TODO create NodesBinaryHeap.config
  bool bIsTieBreakMaxTime;

  void MoveUp(size_t NodeIndex);
  void MoveDown(size_t NodeIndex);

public:
  NodesBinaryHeap() = delete;
  NodesBinaryHeap(bool InIsTieBreakMaxTime);

  // Returns true if the first node is greater than the second one
  bool Compare(const NodeType& First, const NodeType& Second) const;

  NodeType* PopMin();

  void Insert(NodeType& NewNode);

  void ImproveTime(NodeType& ChangedNode, float NewMinTime);

  size_t Size() const;
};

template<typename CellType>
NodesBinaryHeap<CellType>::NodesBinaryHeap(bool InIsTieBreakMaxTime)
  : Nodes{ nullptr } 
  , bIsTieBreakMaxTime(InIsTieBreakMaxTime)
{
  Nodes.reserve(HEAP_START_CAPACITY);
}

template<typename CellType>
void NodesBinaryHeap<CellType>::MoveUp(size_t NodeIndex)
{
  for (size_t parentIndex = (NodeIndex >> 1);
    parentIndex && Compare(*Nodes[parentIndex], *Nodes[NodeIndex]);
    NodeIndex >>= 1, parentIndex >>= 1)
  {
    std::swap(Nodes[parentIndex]->HeapIndex, Nodes[NodeIndex]->HeapIndex);
    std::swap(Nodes[parentIndex], Nodes[NodeIndex]);
  }
}

template<typename CellType>
void NodesBinaryHeap<CellType>::MoveDown(size_t NodeIndex)
{
  for (size_t MinChildIndex = NodeIndex << 1; MinChildIndex < Nodes.size(); MinChildIndex = NodeIndex << 1)
  {
    if (MinChildIndex + 1 < Nodes.size() && Compare(*Nodes[MinChildIndex], *Nodes[MinChildIndex + 1]))
    {
      ++MinChildIndex;
    }

    NodeType& CurrentNode = *Nodes[NodeIndex];
    NodeType& MinChild = *Nodes[MinChildIndex];
    if (!Compare(CurrentNode, MinChild))
    {
      return;
    }

    std::swap(CurrentNode.HeapIndex, MinChild.HeapIndex);
    std::swap(Nodes[NodeIndex], Nodes[MinChildIndex]);
    NodeIndex = MinChildIndex;
  }
}

template<typename CellType>
void NodesBinaryHeap<CellType>::Insert(NodeType& NewNode)
{
  NewNode.HeapIndex = Nodes.size();
  Nodes.emplace_back(&NewNode);
  MoveUp(NewNode.HeapIndex);
}

template<typename CellType>
void NodesBinaryHeap<CellType>::ImproveTime(NodeType& ChangedNode, float NewMinTime)
{
  ChangedNode.MinTime = NewMinTime;
  MoveUp(ChangedNode.HeapIndex);
}

template<typename CellType>
Node<CellType>* NodesBinaryHeap<CellType>::PopMin()
{
  if (Size() == 0)
  {
    return nullptr;
  }

  NodeType* Result = Nodes[1];
  std::swap(Nodes[1], Nodes.back());
  Nodes.pop_back();

  if (Size() > 0)
  {
    Nodes[1]->HeapIndex = 1;
    MoveDown(1);
  }

  return Result;
}

template<typename CellType>
size_t NodesBinaryHeap<CellType>::Size() const
{
  assert(Nodes.size() >= 1);
  return Nodes.size() - 1;
}

template<typename CellType>
bool NodesBinaryHeap<CellType>::Compare(const NodeType& First, const NodeType& Second) const
{
  float FirstFullTime = First.MinTime + First.HeursticToGoal;
  float SecondFullTime = Second.MinTime + Second.HeursticToGoal;

  if (FirstFullTime == SecondFullTime)
  {
    return bIsTieBreakMaxTime == (First.MinTime < Second.MinTime);
  }

  return FirstFullTime > SecondFullTime;
}
