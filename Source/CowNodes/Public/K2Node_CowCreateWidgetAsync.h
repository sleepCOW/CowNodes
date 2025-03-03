// Copyright (c) 2025 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Editor/UMGEditor/Private/Nodes/K2Node_CreateWidget.h"
#include "K2Node_CowCreateWidgetAsync.generated.h"

/**
 * Add copyright notice
 * 
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
 *		  2. No support for conflicting names e.g. ExposedVar named "SoftWidgetClass" will cause error for node compilation
 *		  3. Even if soft-ref already loaded you will have 1 frame delay because of LoadAsset (#TODO)
 *		  4. In the editor the node holds hard-ref to the WidgetClass (This is required for pin generation and proper reloading when WidgetClass changes)
 */
UCLASS()
class COWNODES_API UK2Node_CowCreateWidgetAsync : public UK2Node_CreateWidget
{
	GENERATED_BODY()
public:
	
	// UK2Node_ConstructObjectFromClass BEGIN
	virtual bool IsSpawnVarPin(UEdGraphPin* Pin) const override;
	virtual void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins = nullptr) override;
	// UK2Node_ConstructObjectFromClass END
	
	// Life cycle BEGIN
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	virtual void ReconstructNode() override;
	virtual void PostReconstructNode() override;
	// Life cycle END

	virtual void AllocateDefaultPins() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* ChangedPin) override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
    virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;

	// COMPILATION BEGIN
	
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	UEdGraphPin* GenerateAssignments(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, class UK2Node_CallFunction* CallBeginSpawnNode, UEdGraphNode* SpawnNode, UEdGraphPin* CallBeginResult, const UEdGraphPin* CallBeginClassInput) const;
	
	// If we happen to connect SoftWidgetClass via Link to another pin (meaning at runtime the class may be different)
	// Give a note for any default values that aren't changed from the base class
	// @note: Must be used only when SoftWidgetClass is connected to another pin
	void ValidateSpawnVarPins(const FKismetCompilerContext& CompilerContext) const;
	bool ValidateSpawnVarPinsNameConflicts(const FKismetCompilerContext& CompilerContext) const;

	// Generates a series of nodes to convert SoftClassPin to SoftObjectReference
    // that can be hooked into AsyncLoadAsset.Asset (Soft Object Reference)
    // @returns last pin in chain in unconnected state
    UEdGraphPin* GenerateConvertToSoftObjectRef(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* SoftClassPin);

	// COMPILATION END
	
	virtual FName GetCornerIcon() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetBaseNodeTitle() const override;
	virtual FText GetNodeTitleFormat() const override;

	// Different helpers
	void TryCreateOnWidgetCreatedPin();
	void OnSoftWidgetClassChanged();
	void UnbindFromBlueprintChange();
    bool IsSoftWidgetClassConnected() const;
    UClass* GetClassToSpawn() const;
	UEdGraphPin* GetSoftWidgetPin() const;
	FORCEINLINE TArray<UEdGraphPin*, TInlineAllocator<2>> GetInternalPins() const
	{
		return { FindPinChecked(WidgetClass), FindPinChecked(SoftWidgetClass) };
	}
	FORCEINLINE static TArray<FName, TInlineAllocator<2>> GetInternalPinNames()
	{
		return { WidgetClass, SoftWidgetClass };
	}
	
    static bool IsAnyInputExecPinsConnected(const TArray<UEdGraphPin*>& Pins, UEdGraphPin*& OutFirstUnconnectedPin);

    // Including possibility that Child is Native
    static UClass* GetFirstNativeClass(UClass* Child);

#if WITH_EDITORONLY_DATA
	// Used only in editor time to generate pins correctly
	UPROPERTY()
	TObjectPtr<UClass> WidgetClassToSpawn;

	TWeakObjectPtr<class UBlueprint> WeakWidgetClassBlueprint;
	FDelegateHandle OnWidgetBlueprintChangedHandle;
#endif
	
	// This node pins
	static inline const FName WidgetClass = TEXT("Class");
	static inline const FName WidgetCreated = TEXT("WidgetCreated");
	static inline const FName SoftWidgetClass = TEXT("SoftWidgetClass");

	// UK2Node_LoadAsset
	// Load asset node pins (can't use getters because they're exposed only in 5.5)
	static inline const FName LoadAsset_Input = TEXT("Asset");
	static inline const FName LoadAsset_Output = TEXT("Object");

	// UWidgetBlueprintLibrary::Create
	static inline const FName Create_InputWorldContextObject = TEXT("WorldContextObject");
	static inline const FName Create_InputWidgetType = TEXT("WidgetType");
	static inline const FName Create_InputOwningPlayer = TEXT("OwningPlayer");
};
