// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CowGetAllActorsOfClass.generated.h"

/** 
 * GetAllActors of class with no hard-ref and return array type promotion to the native class
 */
UCLASS()
class COWNODES_API UK2Node_CowGetAllActorsOfClass : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UK2Node Interface
	virtual FText GetMenuCategory() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UK2Node Interface.

	// Life cycle BEGIN
	virtual void PostLoad() override;
	virtual void PostReconstructNode() override;
	// Life cycle END

	virtual void AllocateDefaultPins() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* ChangedPin) override;

	// COMPILATION BEGIN
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	// COMPILATION END

	// Different helpers
	void OnActorClassChanged();
	UClass* GetNativeClassFromInput() const;
	
	// This node pins
	static inline const FName WorldContextObjectName = TEXT("WorldContextObject");
	static inline const FName ActorClassName = TEXT("ActorClass");
	static inline const FName OutActorsName = TEXT("OutActors");
};
