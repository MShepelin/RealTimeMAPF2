#pragma once

#include "CoreMinimal.h"
#include "HogUtils/ScenarioLoader.h"
#include "SearchTypes.h"

#include "MAPFHelpers.generated.h"

UCLASS()
class UAnalyticsBlueprintLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

  UFUNCTION(BlueprintCallable)
  static TArray<FAgentTask> GetAgentTasksFromHogFile(FString FileName, int TasksNum);
};
