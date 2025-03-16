#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_CowFastForEach.generated.h"


/**
 *  TODO:
 *  - Make it working :)
 *  - Add version where element is passed by reference so loop can modify it in blueprints
 *      see UK2Node_GetArrayItem::GetMenuActions
 */
UCLASS()
class COWNODES_API UK2Node_CowFastForEach : public UK2Node
{
    GENERATED_BODY()

public:
    virtual void AllocateDefaultPins() override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    // COMPILATION BEGIN

    virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    // COMPILATION END
    
    static inline const FName PN_Input_Array = FName(TEXT("Array"));

    static inline const FName PN_Output_LoopBody = FName(TEXT("Loop Body"));
    static inline const FName PN_Output_ArrayElement = FName(TEXT("Array Element"));
    static inline const FName PN_Output_ArrayIndex = FName(TEXT("Array Index"));

    
    static void GetArrayTypeDependentPins(class UK2Node_CallArrayFunction* CallArrayFunction, TArray<UEdGraphPin*>& OutPins);
    static void PropagateArrayTypeInfo(class UK2Node_CallArrayFunction* CallArrayFunction, const UEdGraphPin* SourcePin);
    
protected:
    /** Constructing FText strings can be costly, so we cache the node's title */
    FNodeTextCache CachedNodeTitle;
};
