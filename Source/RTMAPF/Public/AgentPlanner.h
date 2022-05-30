#pragma once

#include "Agent.h"
#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "Pathfinding.h"
#include "SearchTypes.h"
#include "SpaceWrapper.h"

#include <list>
#include <memory>

struct ReplanChanges
{
	bool ReplanSeccess;
	std::vector<Node<Area>> ReversedPath;
};

struct FAdaptivePath
{
protected:
	UAgent* Agent;

	std::shared_ptr<SpaceTime> Space;
	mutable std::vector<Node<Area>> ReversedPath;
	size_t NextNodeIndex = 1;
	
	float Depth = 0;
	float CurrentTime = 0;
	float InactivityDelay = 1.f;

	FShape AgentShapeCapture;

	TFuture<ReplanChanges> ReplanResult;

	mutable FCriticalSection PathSync;

private:
	inline const Node<Area>& GetNextNode() const;
	inline const Node<Area>& GetPreviousNode() const;

	void ClearAreasWithPath(const std::vector<Node<Area>>& InReversedPath) const;
	void FillAreasWithPath(const std::vector<Node<Area>>& InReversedPath) const;

public:
	FAdaptivePath() = default;
	FAdaptivePath(UAgent* InAgent, std::shared_ptr<SpaceTime> Space, float Depth, float CurrentTime, float InactivityDelay = 1.f);
	FAdaptivePath(FAdaptivePath&& Other);

	bool Replan(float InDepth);
	bool CheckForUpdate();
	void MoveTimeBy(float DeltaTime);

	bool IsAnyPathReady() const
	{
		FScopeLock PathLock(&PathSync);
		return ReversedPath.size() > 0;
	}

	UAgent* GetAgent() const
	{
		return Agent;
	}

	FVector GetCurrentLocation(ASpace* SpaceWrapper) const;
	FPathPoint GetNextMove(ASpace* SpaceWrapper) const;

	~FAdaptivePath();
};
