// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "K2Node_CowGetAllActorsOfClass.generated.h"

/** #TODO: Change
 * Improved version of Epic's CreateWidget and CreateWidgetAsync
 * 
 * Combining best of both words with an attempt for general QoL improvements:
 *  - Doesn't introduce hard-ref to the selected widget class (similar to CreateWidgetAsync)
 *  - Shows any ExposedOnSpawn parameters (similar to CreateWidget)
 *  - Automatically adds new ExposedOnSpawn variables (CreateWidget doesn't handle it)
 *  - Widget class pin supports linked properties and propagates their class:
 *      Let's say you created a soft class property MyClass of type WBP_BaseWidget with exposed member Text (so only WBP_BaseWidget and derived classes can be selected)
 *      If you connect MyClass property to the node it will properly do:
 *          1. Display any exposed members, in our case Text will be shown
 *          2. Make a return type of the node to be WBP_BaseWidget
 *              Because you already have hard-ref due to pin type (Yep, SoftClass property to a type X is a hard-ref to the X)
 *  - Return value is always the best possible option that won't cause a hard-ref
 *		either a first native class or widget class if linked to a pin which introduced hard-ref
 *  - Proper auto-wiring for ReturnType to avoid common mistake of using "Then" pin instead of "OnWidgetCompleted"
 *  - Generates "user-friendly" notes when you try to make something that could be an error
 *		(e.g. introduction of hard-ref, keeping default values of base class when you hooked pin of a base class and it may differ at runtime)
 *	- Fixed FKismetCompilerUtilities::GenerateAssignmentNodes create hard-ref through DynamicCast for native properties with BlueprintSetter
 *		See FCowCompilerUtilities::GenerateAssignmentNodes for implementation details
 * 
 * For implementation details see ExpandNode (but shortly it replaces the node with LoadAsset -> Cast to UUserWidget -> Set var calls)
 * 
 * @note: Known limitations:
 *		  1. Works only with EventGraph/Macro (because async)
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
	virtual void BeginDestroy() override;
	virtual void ReconstructNode() override;
	virtual void PostReconstructNode() override;
	// Life cycle END

	virtual void AllocateDefaultPins() override;

	// COMPILATION BEGIN
	
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	// COMPILATION END

	// Different helpers
	UClass* GetNativeClassFromInput() const;
	
	// This node pins
	static inline const FName WorldContextObjectName = TEXT("WorldContextObject");
	static inline const FName ActorClassName = TEXT("ActorClass");
	static inline const FName OutActorsName = TEXT("OutActors");
};
