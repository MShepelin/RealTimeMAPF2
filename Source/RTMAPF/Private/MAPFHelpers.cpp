#include "MAPFHelpers.h"

TArray<FAgentTask> UAnalyticsBlueprintLibrary::GetAgentTasksFromHogFile(FString FileName, int TasksNum)
{
  ScenarioLoader Loader(TCHAR_TO_ANSI(*FileName));
  if (Loader.GetNumExperiments() < TasksNum)
  {
    UE_LOG(LogTemp, Warning, TEXT("Tasks Num is too big for filename %s, have only %d out of requested %d"), *FileName, Loader.GetNumExperiments(), TasksNum)
    TasksNum = Loader.GetNumExperiments();
  }

  TArray<FAgentTask> Result;
  for (int i = 0; i < TasksNum; ++i)
  {
    auto Experiment = Loader.GetNthExperiment(i);
    Result.Add({ 
      FPoint(Experiment.GetStartX(), Experiment.GetStartY()),
      FPoint(Experiment.GetGoalX(), Experiment.GetGoalY())
    });
  }

  return Result;
}
