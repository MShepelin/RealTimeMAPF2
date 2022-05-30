#pragma once

// TODO Move to config?

#include "CoreMinimal.h"

#include <functional>
#include <inttypes.h>
#include <set>
#include <unordered_map>
#include <vector>

#include "SearchTypes.generated.h"

template <class _Ty, class _Alloc = std::allocator<_Ty>>
using ArrayType = std::vector<_Ty, _Alloc>;

template <class _Kty, class _Ty, class _Hasher = std::hash<_Kty>, class _Keyeq = std::equal_to<_Kty>,
  class _Alloc = std::allocator<std::pair<const _Kty, _Ty>>>
using MapType = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq, _Alloc>;

template <class _Kty, class _Pr = std::less<_Kty>, class _Alloc = std::allocator<_Kty>>
using SetType = std::set<_Kty, _Pr, _Alloc>;

// The difinitions of MAKE_HASHABLE and HashCombine are borrowed from:
// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x

#define MAKE_HASHABLE(T, ...) \
    namespace std {\
        template<> struct hash<T> {\
            size_t operator()(const T &Type) const {\
                size_t Ret = 0;\
                HashCombine(Ret, __VA_ARGS__);\
                return Ret;\
            }\
        };\
    }

inline void HashCombine(std::size_t& seed) {};

template <typename T, typename... Rest>
inline void HashCombine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> Hasher;
  seed ^= Hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  HashCombine(seed, rest...);
}

enum class Access : uint8_t
{
  Accessable = 1,
  Inaccessable = 0
};

#define START_TIME 0.f

#define EPSILON 1e-5

struct Area;

USTRUCT(BlueprintType)
struct FPoint
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int X = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int Y = 0;

  FPoint operator+(const FPoint& Other) const
  {
    return { X + Other.X, Y + Other.Y };
  }

  FPoint operator-(const FPoint& Other) const
  {
    return { X - Other.X, Y - Other.Y };
  }

  bool operator==(const FPoint& Other) const
  {
    return X == Other.X && Y == Other.Y;
  }

  FPoint operator*(const FPoint& Other) const
  {
    return FPoint(X * Other.X, Y * Other.Y);
  }

  FPoint(int InX, int InY)
    : X(InX)
    , Y(InY)
  { }

  FPoint()
    : X(0)
    , Y(0)
  { }

  explicit FPoint(const Area& Area);
};

MAKE_HASHABLE(FPoint, Type.X, Type.Y);

template<typename CellType>
struct Node
{
  CellType Cell;
  // If HeursticToGoal < 0 the node is in a "close" list, else it's in an "open" list
  // If MinTime < 0 the node is currently inaccessable, 
  // else MinTime = current min time to reach the node
  float MinTime, HeursticToGoal;
  size_t HeapIndex = 0;
  Node<CellType>* Parent = nullptr;
  float ArrivalCost = 0;

  Node();
  Node(CellType InCell);
  Node(CellType InCell, float InMinTime, float InHeuristic = -1, float InArrivalCost = 0);

  inline void MarkClosed() { HeursticToGoal = -1; }
};

template<typename CellType>
Node<CellType>::Node(CellType InCell)
  : Cell(InCell)
  , MinTime(0)
  , HeursticToGoal(-1)
{ }

template<typename CellType>
Node<CellType>::Node(CellType InCell, float InMinTime, float InHeuristic, float InArrivalCost)
  : Cell(InCell)
  , MinTime(InMinTime)
  , HeursticToGoal(InHeuristic)
  , ArrivalCost(InArrivalCost)
{ }

template<typename CellType>
Node<CellType>::Node()
  : Cell()
  , MinTime(-1)
  , HeursticToGoal(-1)
{ }

USTRUCT(BlueprintType)
struct FPathPoint
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float Time;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FVector Destionation;
};

USTRUCT(BlueprintType)
struct RTMAPF_API FAgentTask
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FPoint Start;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FPoint Goal;
};
