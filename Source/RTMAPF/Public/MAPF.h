#pragma once

#include "Agent.h"
#include "AgentPlanner.h"
#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "Misc/ScopeLock.h"
#include "Pathfinding.h"
#include "SearchTypes.h"
#include "SpaceWrapper.h"

#include <list>
#include <memory>

#include "MAPF.generated.h"

UCLASS()
class RTMAPF_API UMultiagentPathfinder : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	ASpace* SpaceWrapper;

	std::shared_ptr<SpaceTime> Space;

	// TODO maybe make unique ptr
	TMap<int, FAdaptivePath> AgentPaths;

	UPROPERTY()
	TArray<UAgent*> PendingToAdd;
	// Order in which IDs shoud be processed. 
	// Can have improper IDs
	TDoubleLinkedList<int> Order;

	TOptional<int> CurrentlyReplanning;
	bool PendingRemove = false;
	bool ReplanningFreshAgent = false;
	bool ReplanRepeat = false;

	float CurrentTime = 0;
	float Depth = 0;

	int MaxAgentID = 0;

	mutable FCriticalSection AccessAgentPaths;

public:
	UMultiagentPathfinder();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void SetSpace(ASpace* InSpaceWrapper);

	UFUNCTION(BlueprintCallable)
	ASpace* GetSpaceWrapper() const
	{
		return SpaceWrapper;
	}

	UFUNCTION(BlueprintCallable)
	void SetDepth(float InDepth);

	UFUNCTION(BlueprintCallable)
	void Reset();

	UFUNCTION(BlueprintCallable)
	void Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void AddAgent(UAgent* Agent);

	UFUNCTION(BlueprintCallable)
	void RemoveAgent(int ID);

	UFUNCTION(BlueprintCallable)
  FVector GetCurrentLocation(int ID) const;
	
	UFUNCTION(BlueprintCallable)
  FPathPoint GetNextMove(int ID) const;

	UFUNCTION(BlueprintCallable)
	void ForceReplan(int ID);

	UFUNCTION(BlueprintCallable)
	float GetCurrentTime() const;

	std::shared_ptr<SpaceTime> GetSpace() const { return Space; };
};
