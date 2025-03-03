// Copyright (c) 2025 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class FKismetCompilerContext;

namespace FCowCompilerUtilities
{
	COWNODES_API const UClass* GetFirstNativeClass(const UClass* Child);
	COWNODES_API UClass* GetFirstNativeClass(UClass* Child);
	
	/**
	 * Copy-paste of 5.5.3 FKismetCompilerUtilities::GenerateAssignmentNodes
	 *
	 * Changes:
	 *	1. Fixed hard-ref introduced when generating assignments for BlueprintSetter
	 */
	COWNODES_API UEdGraphPin* GenerateAssignmentNodes(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UK2Node* CallBeginSpawnNode, UEdGraphNode* SpawnNode, UEdGraphPin* CallBeginResult, const UClass* ForClass, const UEdGraphPin* CallBeginClassInput = nullptr);
}