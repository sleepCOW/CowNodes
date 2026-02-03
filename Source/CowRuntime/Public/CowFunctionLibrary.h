// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CowFunctionLibrary.generated.h"

UCLASS()
class COWRUNTIME_API UCowFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// #TODO: Add comment
	UFUNCTION(BlueprintCallable, Category = "Cow|Utilities", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ActorClass"))
	static void CowGetAllActorsOfClass(const UObject* WorldContextObject, TSoftClassPtr<AActor> ActorClass, TArray<AActor*>& OutActors);
};