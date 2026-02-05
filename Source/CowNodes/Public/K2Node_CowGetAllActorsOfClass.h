// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CowGetAllActorsOfClass.generated.h"

class UToolMenu;
class UGraphNodeContextMenuContext;

/** 
 * GetAllActors of class with no hard-ref and return array type promotion to the native class
 *
 * Use Convert to Multi/Single version using context menu
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
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
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
	const FName GetOutPinName() const;
	void OnActorClassChanged();
	UClass* GetNativeClassFromInput() const;
	void ToggleNodeOutput();
	static const FText GetConvertContextActionName(const bool InOutputAsArray);

	// By default create Array output pin
	// If false -> output single item, so GetActorOfClassVersion
	UPROPERTY()
	bool bOutputAsArray = true;
	
	// This node pins
	static inline const FName WorldContextObjectName = TEXT("WorldContextObject");
	static inline const FName ActorClassName = TEXT("ActorClass");
	static inline const FName OutActorsName = TEXT("OutActors");
	static inline const FName OutActorName = TEXT("OutActor");
};
