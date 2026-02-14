// Copyright (c) 2025 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#include "K2Node_CowGetAllActorsOfClass.h"

// Engine
#include "BlueprintCompilationManager.h"
#include "CowCompilerUtilities.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"

// Cow
#include "CowFunctionLibrary.h"

#define LOCTEXT_NAMESPACE "Cow"

void UK2Node_CowGetAllActorsOfClass::PostLoad()
{
	Super::PostLoad();

	OnActorClassChanged();
}

void UK2Node_CowGetAllActorsOfClass::PostReconstructNode()
{
	Super::PostReconstructNode();

	OnActorClassChanged();
}

void UK2Node_CowGetAllActorsOfClass::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	FEdGraphPinType WorldContextPinType;
	WorldContextPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	WorldContextPinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;
	UEdGraphPin* WorldContextPin = CreatePin(EGPD_Input, WorldContextPinType, WorldContextObjectName);
	WorldContextPin->bHidden = true;

	FEdGraphPinType ActorClassPinType;
	ActorClassPinType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
	ActorClassPinType.PinSubCategoryObject = AActor::StaticClass();
	CreatePin(EGPD_Input, ActorClassPinType, ActorClassName);

	FEdGraphPinType OutActorsPinType;
	OutActorsPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	if (bOutputAsArray)
	{
		OutActorsPinType.ContainerType = EPinContainerType::Array;
	}
	OutActorsPinType.PinSubCategoryObject = AActor::StaticClass();
	const FName OutActorPinName = bOutputAsArray ? OutActorsName : OutActorName;
	CreatePin(EGPD_Output, OutActorsPinType, OutActorPinName);
}

void UK2Node_CowGetAllActorsOfClass::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	if (ChangedPin == FindPinChecked(ActorClassName, EGPD_Input))
	{
		OnActorClassChanged();
	}
}

void UK2Node_CowGetAllActorsOfClass::PinConnectionListChanged(UEdGraphPin* ChangedPin)
{
	if (ChangedPin == FindPinChecked(ActorClassName, EGPD_Input))
	{
		OnActorClassChanged();
	}
}

void UK2Node_CowGetAllActorsOfClass::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (GetNativeClassFromInput() == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CowGetAllActorsOfClass_Error", "Cow Get All Actors Of Class node @@ must have a class specified!").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	UK2Node_CallFunction* Call_GetAllActorsOfClass = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	const FName GetFunctionName = bOutputAsArray ? GET_FUNCTION_NAME_CHECKED(UCowFunctionLibrary, CowGetAllActorsOfClass) 
												 : GET_FUNCTION_NAME_CHECKED(UCowFunctionLibrary, CowGetActorOfClass);
	Call_GetAllActorsOfClass->FunctionReference.SetExternalMember(GetFunctionName, UCowFunctionLibrary::StaticClass());
	Call_GetAllActorsOfClass->AllocateDefaultPins();

	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), 
											   *Call_GetAllActorsOfClass->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), 
											   *Call_GetAllActorsOfClass->GetThenPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(WorldContextObjectName, EGPD_Input),
											   *Call_GetAllActorsOfClass->FindPinChecked(WorldContextObjectName, EGPD_Input));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(ActorClassName, EGPD_Input), 
											   *Call_GetAllActorsOfClass->FindPinChecked(ActorClassName, EGPD_Input));

	UEdGraphPin* This_OutActorsPin = FindPinChecked(GetOutPinName(), EGPD_Output);
	UEdGraphPin* Output_OutActorsPin = Call_GetAllActorsOfClass->FindPinChecked(GetOutPinName(), EGPD_Output);
	Output_OutActorsPin->PinType = This_OutActorsPin->PinType; // (Type match required to connect pins)
	CompilerContext.MovePinLinksToIntermediate(*This_OutActorsPin, 
											   *Output_OutActorsPin);

	BreakAllNodeLinks();
}

FText UK2Node_CowGetAllActorsOfClass::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
}

FText UK2Node_CowGetAllActorsOfClass::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return bOutputAsArray ? FText(LOCTEXT("CowGetAllActorsOfClassNodeTitle", "Cow Get All Actors Of Class")) :
							FText(LOCTEXT("CowGetActorOfClassNodeTitle", "Cow Get Actor Of Class"));
}

void UK2Node_CowGetAllActorsOfClass::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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
		// CowGetAllActorsOfClass
		UBlueprintNodeSpawner* DefaultNodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(DefaultNodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, DefaultNodeSpawner);

		// Separate registration for CowGetActorOfClass version
		auto PostSpawnSetupLambda = [](UEdGraphNode* InNewNode, bool bIsTemplateNode)
		{
			UK2Node_CowGetAllActorsOfClass* CowGetActorOfClass = CastChecked<UK2Node_CowGetAllActorsOfClass>(InNewNode);
			CowGetActorOfClass->bOutputAsArray = false;
		};
		UBlueprintNodeSpawner::FCustomizeNodeDelegate PostSpawnDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(PostSpawnSetupLambda);
		UBlueprintNodeSpawner* SingleVersionNode = UBlueprintNodeSpawner::Create(GetClass(), nullptr, PostSpawnDelegate);
		check(SingleVersionNode != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, SingleVersionNode);
	}
}

void UK2Node_CowGetAllActorsOfClass::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	FToolMenuSection& Section = Menu->AddSection("K2Node_CowGetAllActorsOfClass", LOCTEXT("FunctionHeader", "Function"));

	Section.AddMenuEntry(
		TEXT("ToggleNodeOutput"),
		GetConvertContextActionName(bOutputAsArray),
		GetConvertContextActionName(bOutputAsArray),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateUObject(const_cast<UK2Node_CowGetAllActorsOfClass*>(this), &UK2Node_CowGetAllActorsOfClass::ToggleNodeOutput),
			FCanExecuteAction(),
			FIsActionChecked()
		)
	);
}

UClass* UK2Node_CowGetAllActorsOfClass::GetNativeClassFromInput() const
{
    UEdGraphPin* ActorClassPin = FindPinChecked(ActorClassName, EGPD_Input);

    // If ActorClassPin isn't connected to anything and not empty we should use Path written in DefaultValue
	if (!ActorClassPin->DefaultValue.IsEmpty() && ActorClassPin->LinkedTo.Num() == 0)
	{
        FSoftObjectPath SoftActorClass = ActorClassPin->DefaultValue;

		// If class is blueprint try to get native parent from metadata to avoid loading class here
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData AssetData;
		if (AssetRegistryModule.Get().TryGetAssetByObjectPath(SoftActorClass, AssetData) == UE::AssetRegistry::EExists::Exists)
		{
			UClass* NativeParentClass = nullptr;
			FString ParentClassName;
			if(!AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, ParentClassName))
			{
				// By the looks of it we shouldn't rely on it but all places that use NativeParentClassPath do, so let's be safe
				AssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassName);
			}
			if(!ParentClassName.IsEmpty())
			{
				UObject* Outer = nullptr;
				ResolveName(Outer, ParentClassName, false, false);
				NativeParentClass = FindObject<UClass>(Outer, *ParentClassName);
			}
			return NativeParentClass;
		}
		else
		{
			UClass* NativeClass = FindObject<UClass>(SoftActorClass.GetAssetPath(), true);
			return NativeClass;
		}
	}
	else if (ActorClassPin->LinkedTo.Num())
	{
		// If class pin connected to anything that means that our type is propagated
		// And it's safe to use it (because hard-ref to the type is already created by the connection)
		// We're not the one to blame in such a situation :)
		UEdGraphPin* ClassSource = ActorClassPin->LinkedTo[0];
		return Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get());
	}
    
    return nullptr;
}

void UK2Node_CowGetAllActorsOfClass::OnActorClassChanged()
{
	UEdGraphPin* ActorClassPin = FindPinChecked(ActorClassName, EGPD_Input);

    // Fix our return type
    UEdGraphPin* OutActorsPin = FindPinChecked(GetOutPinName(), EGPD_Output);
	OutActorsPin->PinType.PinSubCategoryObject = GetNativeClassFromInput();
}

void UK2Node_CowGetAllActorsOfClass::ToggleNodeOutput()
{
	const FText TransactionTitle = GetConvertContextActionName(bOutputAsArray);

	// Remove previous version output pin
	RemovePin(FindPinChecked(GetOutPinName(), EGPD_Output));
	bOutputAsArray = !bOutputAsArray;
	
	FScopedTransaction Transaction(TransactionTitle);
	Modify();
	ReconstructNode();
}

const FText UK2Node_CowGetAllActorsOfClass::GetConvertContextActionName(const bool InOutputAsArray)
{
	return InOutputAsArray ? LOCTEXT("ConvertNodeToSingle", "Convert To Single Actor") : LOCTEXT("ConvertNodeToMulti", "Convert Get To All Actors");
}

const FName UK2Node_CowGetAllActorsOfClass::GetOutPinName() const
{
	return bOutputAsArray ? OutActorsName : OutActorName;
}

#undef LOCTEXT_NAMESPACE