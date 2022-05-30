#pragma once

#include "CoreMinimal.h"
#include "Moves.h"
#include "SearchTypes.h"
#include "Shapes.h"
#include "Space.h"

#include <cassert>
#include <cmath>

#include "Agent.generated.h"

class MovesTestSegment : public MoveComponent<Area>, public MoveComponent<FPoint>
{
protected:
  float Depth;
  std::shared_ptr<ShapeSpace> Space;
  ArrayType<MoveDelta<FPoint>> Moves;

public:
  virtual ArrayType<MoveDelta<Area>> FindValidMoves(const Node<Area>& Node) override;

  virtual ArrayType<MoveDelta<FPoint>> FindValidMoves(const Node<FPoint>& Node) override;

  MovesTestSegment(ArrayType<MoveDelta<FPoint>>& InMoves, std::shared_ptr<ShapeSpace> InSpace, float InDepth)
    : Depth(InDepth)
    , Space(InSpace)
    , Moves(InMoves)
  {}
};

class UMultiagentPathfinder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnection);

UCLASS(BlueprintType, Blueprintable)
class RTMAPF_API UAgent : public UObject
{
  GENERATED_BODY()

protected:
  UPROPERTY()
  int AgentID = -1;

  UPROPERTY(EditDefaultsOnly)
  FShape Shape;

  UPROPERTY(EditDefaultsOnly)
  TArray<FPointMove> Moves = {
    FPointMove{ 1.f, {0, 1}},
    FPointMove{ 1.f, {0, -1}},
    FPointMove{ 1.f, {1, 0}},
    FPointMove{ 1.f, {-1, 0}},
    FPointMove{ std::sqrt(2.f), {1, 1}},
    FPointMove{ std::sqrt(2.f), {-1, -1}},
    FPointMove{ std::sqrt(2.f), {1, -1}},
    FPointMove{ std::sqrt(2.f), {-1, 1}},
  };

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  FPoint Start;

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  FPoint Goal;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
  float SpeedModifier = 1.f;

  bool bIsConnected = false;

  mutable FCriticalSection PropertiesSync;

private:
  UMultiagentPathfinder* GetMAPF() const;

public:
  void GetPropertiesSafe(
    int& OuterID,
    FPoint& OuterStart,
    FPoint& OuterGoal,
    FShape& OuterShape,
    ArrayType<MoveDelta<FPoint>>& OuterMoves,
    float& OuterSpeed
  ) const;

  UPROPERTY(BlueprintAssignable)
  FOnConnection OnConnection;

  UPROPERTY(BlueprintAssignable)
  FOnConnection OnConnectionFailure;

  UPROPERTY(BlueprintAssignable)
  FOnConnection OnReplan;

  UFUNCTION(BlueprintCallable)
  void InitFrom(FAgentTask Task);

  FPoint GetStartSafe() const
  {
    FScopeLock g(&PropertiesSync);

    return Start;
  }

  UFUNCTION(BlueprintCallable)
  int GetIDUnsafe() const
  {
    return AgentID;
  }

  UFUNCTION(BlueprintCallable)
  void Disconnect();

  void SetIDUnsafe(int NewID)
  {
    AgentID = NewID;
  }

  void MarkConnection(bool NewConnectionStatus = true)
  {
    bIsConnected = NewConnectionStatus;
    if (bIsConnected)
    {
      OnConnection.Broadcast();
    }
  }

  void ConnectionFailed()
  {
    OnConnectionFailure.Broadcast();
    ConditionalBeginDestroy();
  }

  UFUNCTION(BlueprintCallable)
  bool IsConnected() const
  {
    return bIsConnected;
  }

  virtual void BeginDestroy() override;

  UFUNCTION(BlueprintCallable)
  FVector GetCurrentLocation() const;

  UFUNCTION(BlueprintCallable)
  FPathPoint GetNextMove() const;

  UFUNCTION(BlueprintCallable)
  void ChangeGoal(FPoint NewGoal);
};
