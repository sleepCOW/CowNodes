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

void UK2Node_CowGetAllActorsOfClass::PostReconstructNode()
{
	Super::PostReconstructNode();
}

void UK2Node_CowGetAllActorsOfClass::PostLoad()
{
	UE_LOG(LogTemp, Log, TEXT("PostLoad"));
	
	Super::PostLoad();
}

void UK2Node_CowGetAllActorsOfClass::BeginDestroy()
{
	UE_LOG(LogTemp, Log, TEXT("BeginDestroy"));
	
	Super::BeginDestroy();
}

void UK2Node_CowGetAllActorsOfClass::ReconstructNode()
{
	Super::ReconstructNode();

	UE_LOG(LogTemp, Log, TEXT("ReconstructNode"));
}

void UK2Node_CowGetAllActorsOfClass::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	FEdGraphPinType WorldContextPinType;
	WorldContextPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	WorldContextPinType.PinSubCategory = UEdGraphSchema_K2::PSC_Self;
	CreatePin(EGPD_Input, WorldContextPinType, WorldContextObjectName);

	FEdGraphPinType ActorClassPinType;
	ActorClassPinType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
	ActorClassPinType.PinSubCategoryObject = AActor::StaticClass();
	CreatePin(EGPD_Input, ActorClassPinType, ActorClassName);

	// #TODO: Type should be related to input
	FEdGraphPinType OutActorsPinType;
	OutActorsPinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	OutActorsPinType.ContainerType = EPinContainerType::Array;
	OutActorsPinType.PinSubCategoryObject = AActor::StaticClass();
	CreatePin(EGPD_Input, OutActorsPinType, OutActorsName);
}

void UK2Node_CowGetAllActorsOfClass::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (GetNativeClassFromInput() == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CowGetAllActorsOfClass_Error", "Cow Get All Actors Of Class node @@ must have a class specified!").ToString(), this);
		// we break exec links so this is the only error we get, don't want the CreateWidget node being considered and giving 'unexpected node' type warnings
		BreakAllNodeLinks();
		return;
	}

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	UK2Node_CallFunction* Call_GetAllActorsOfClass = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Call_GetAllActorsOfClass->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UCowFunctionLibrary, CowGetAllActorsOfClass), UCowFunctionLibrary::StaticClass());
	Call_GetAllActorsOfClass->AllocateDefaultPins();

	//UEdGraphPin* Call_GetAllActorsOfClass->FindPinChecked(Input_WorldContextObjectName
}

FText UK2Node_CowGetAllActorsOfClass::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
}

FText UK2Node_CowGetAllActorsOfClass::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText(LOCTEXT("CowGetAllActorsOfClassNodeTitle", "Cow Get All Actors Of Class"));
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
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

UClass* UK2Node_CowGetAllActorsOfClass::GetNativeClassFromInput() const
{
    UEdGraphPin* ActorClassPin = FindPinChecked(ActorClassName, EGPD_Input);

    // If SoftWidgetClassPin isn't connected to anything and not empty we should use Path written in DefaultValue
	if (!ActorClassPin->DefaultValue.IsEmpty() && ActorClassPin->LinkedTo.Num() == 0)
	{
        FSoftObjectPath SoftWidget = ActorClassPin->DefaultValue;

		// Looks like shit
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData AssetData;
		if (AssetRegistryModule.Get().TryGetAssetByObjectPath(SoftWidget, AssetData) != UE::AssetRegistry::EExists::Exists)
		{
			return nullptr;
		}

		UClass* NativeParentClass = nullptr;
		FString ParentClassName;
		if(!AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, ParentClassName))
		{
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
	else if (ActorClassPin->LinkedTo.Num())
	{
		UEdGraphPin* ClassSource = ActorClassPin->LinkedTo[0];
		return Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get());
	}
    
    return nullptr;
}

#undef LOCTEXT_NAMESPACE