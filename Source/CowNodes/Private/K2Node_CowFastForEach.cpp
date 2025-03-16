#include "K2Node_CowFastForEach.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "CowNodesRuntimeLibrary.h"
#include "K2Node_CallArrayFunction.h"
#include "KismetCompiler.h"

void UK2Node_CowFastForEach::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	//	Node picture
	//   --------------------------------
	//   | > Exec		    Loop Body > |
	//   | * Array      Array Element * |
	//   |			      Array Index * |
	//   |                  Completed > |
	//   --------------------------------
	
	// Create input pins
	
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	
	UEdGraphNode::FCreatePinParams ArrayPinParams;
	ArrayPinParams.ContainerType = EPinContainerType::Array;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PN_Input_Array, ArrayPinParams);
	
	// Create output pins

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PN_Output_LoopBody);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Completed);
	
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, PN_Output_ArrayElement);
	//CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Int, PN_Output_ArrayIndex);
}

FText UK2Node_CowFastForEach::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		CachedNodeTitle.SetCachedText(FText::FromString(TEXT("Cow Fast For Each Loop")), this);
	}
	return CachedNodeTitle;
}

void UK2Node_CowFastForEach::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_CowFastForEach::GetArrayTypeDependentPins(UK2Node_CallArrayFunction* CallArrayFunction,
                                                       TArray<UEdGraphPin*>& OutPins)
{
	OutPins.Empty();

	UFunction* TargetFunction = CallArrayFunction->GetTargetFunction();
	if (ensure(TargetFunction))
	{
		const FString& DependentPinMetaData = TargetFunction->GetMetaData(FBlueprintMetadata::MD_ArrayDependentParam);
		TArray<FString> TypeDependentPinNameStrs;
		DependentPinMetaData.ParseIntoArray(TypeDependentPinNameStrs, TEXT(","), true);

		TArray<FName> TypeDependentPinNames;
		Algo::Transform(TypeDependentPinNameStrs, TypeDependentPinNames, [](const FString& PinNameStr) { return FName(*PinNameStr); });

		for (TArray<UEdGraphPin*>::TConstIterator it(CallArrayFunction->Pins); it; ++it)
		{
			UEdGraphPin* CurrentPin = *it;
			int32 ItemIndex = 0;
			if (CurrentPin && TypeDependentPinNames.Find(CurrentPin->PinName, ItemIndex))
			{
				OutPins.Add(CurrentPin);
			}
		}
	}

	// Add the target array pin to make sure we get everything that depends on this type
	if(UEdGraphPin* TargetPin = CallArrayFunction->GetTargetArrayPin())
	{
		OutPins.Add(TargetPin);
	}	
}

void UK2Node_CowFastForEach::PropagateArrayTypeInfo(UK2Node_CallArrayFunction* CallArrayFunction,
	const UEdGraphPin* SourcePin)
{
	if( SourcePin )
	{
		const UEdGraphSchema_K2* Schema = CastChecked<UEdGraphSchema_K2>(CallArrayFunction->GetSchema());
		const FEdGraphPinType& SourcePinType = SourcePin->PinType;

		TArray<UEdGraphPin*> DependentPins;
		GetArrayTypeDependentPins(CallArrayFunction, DependentPins);
	
		// Propagate pin type info (except for array info!) to pins with dependent types
		for (UEdGraphPin* CurrentPin : DependentPins)
		{
			if (CurrentPin && CurrentPin != SourcePin)
			{
				FEdGraphPinType& CurrentPinType = CurrentPin->PinType;

				bool const bHasTypeMismatch = (CurrentPinType.PinCategory != SourcePinType.PinCategory) ||
					(CurrentPinType.PinSubCategory != SourcePinType.PinSubCategory) ||
					(CurrentPinType.PinSubCategoryObject != SourcePinType.PinSubCategoryObject);

				if (!bHasTypeMismatch)
				{
					continue;
				}

				if (CurrentPin->SubPins.Num() > 0)
				{
					Schema->RecombinePin(CurrentPin->SubPins[0]);
				}

				CurrentPinType.PinCategory          = SourcePinType.PinCategory;
				CurrentPinType.PinSubCategory       = SourcePinType.PinSubCategory;
				CurrentPinType.PinSubCategoryObject = SourcePinType.PinSubCategoryObject;

				// Reset default values
				if (!Schema->IsPinDefaultValid(CurrentPin, CurrentPin->DefaultValue, CurrentPin->DefaultObject, CurrentPin->DefaultTextValue).IsEmpty())
				{
					Schema->ResetPinToAutogeneratedDefaultValue(CurrentPin);
				}
			}
		}
	}
}

void UK2Node_CowFastForEach::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
	
	// static ENGINE_API void Array_FastForEach(const TArray<int32>& TargetArray, int32& Item);
	UK2Node_CallArrayFunction* CallForEach = CompilerContext.SpawnIntermediateNode<UK2Node_CallArrayFunction>(this, SourceGraph);
	CallForEach->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UCowNodesRuntimeLibrary, Array_FastForEach), UCowNodesRuntimeLibrary::StaticClass());
	CallForEach->AllocateDefaultPins();

	// TEMP #TODO: REMOVE ME AFTER CORRECT COMPILATION IMPL
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *CallForEach->FindPinChecked(UEdGraphSchema_K2::PC_Exec));

	UEdGraphPin* This_InputArray = FindPinChecked(PN_Input_Array);
	UEdGraphPin* CallForEach_InputTargetArray = CallForEach->GetTargetArrayPin();
	PropagateArrayTypeInfo(CallForEach, This_InputArray);
	CompilerContext.MovePinLinksToIntermediate(*This_InputArray, *CallForEach_InputTargetArray);

	//static const FName PN_Input_TargetArray = FName(TEXT("TargetArray"));
	static const FName PN_Output_Item = FName(TEXT("Item"));
	UEdGraphPin* CallForEach_OutputItem = CallForEach->FindPinChecked(PN_Output_Item);

	UEdGraphPin* This_OutputArrayElement = FindPinChecked(PN_Output_ArrayElement);
	CompilerContext.MovePinLinksToIntermediate(*This_OutputArrayElement, *CallForEach_OutputItem);

	// Don't directly compile this node
	BreakAllNodeLinks();
}
