// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#include "CowFunctionLibrary.h"
#include "EngineUtils.h"

void UCowFunctionLibrary::CowGetAllActorsOfClass(const UObject* WorldContextObject, TSoftClassPtr<AActor> ActorClass, TArray<AActor*>& OutActors)
{
	OutActors.Reset();

	// By doing hard mental exercise we were able to deduce that if soft ptr isn't loaded no actors present in world :Einstein:
	if (UClass* LoadedClass = ActorClass.Get())
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			for (TActorIterator<AActor> It{World, LoadedClass}; It; ++It)
			{
				if (AActor* Actor = *It; Actor)
				{
					OutActors.Add(Actor);
				}
			}
		}
	}
}