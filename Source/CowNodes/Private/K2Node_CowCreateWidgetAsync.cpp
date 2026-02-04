// Copyright (c) 2025 Oleksandr "sleepCOW" Ozerov. All rights reserved.


#include "K2Node_CowCreateWidgetAsync.h"

#include "BlueprintCompilationManager.h"
#include "CowCompilerUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_LoadAsset.h"
#include "KismetCompiler.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "Cow"

UEdGraphPin* UK2Node_CowCreateWidgetAsync::GetSoftWidgetPin() const
{
	return FindPinChecked(SoftWidgetClass, EGPD_Input);
}

void UK2Node_CowCreateWidgetAsync::ValidateSpawnVarPins(const FKismetCompilerContext& CompilerContext) const
{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	auto DisplayNoteForPin = [&CompilerContext](UEdGraphPin* Pin)
	{
		CompilerContext.MessageLog.Note(*LOCTEXT("CowCreateWidgetAsync_DefaultValueNote", "@@ pin's default value is equal to base class's default value which may indicate an error.").ToString(), Pin);
	};

	// Blacklist for our internal pins
	auto InternalPins = GetInternalPins();
	
	for (UEdGraphPin* Pin : Pins)
	{
		// We need to check only pins generated for Exposed vars
		// + we only care if default value is edited, any linked pins are out of the validation context
		//	 because their values could be different at runtime
		if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.IsEmpty())
		{
			if (InternalPins.Find(Pin) != INDEX_NONE)
			{
				continue;
			}
			
			FProperty* PropertyForPin = WidgetClassToSpawn->FindPropertyByName(Pin->PinName);
			if (!PropertyForPin)
			{
				continue;
			}
			
			FString DefaultValueErrorString = Schema->IsCurrentPinDefaultValid(Pin);
			if (!DefaultValueErrorString.IsEmpty())
			{
				// Some types require a connection for assignment (e.g. arrays).
				continue;
			}
			// We don't want to generate an assignment node unless the default value 
			// differs from the value in the CDO:
			FString DefaultValueAsString;
			if (!FBlueprintCompilationManager::GetDefaultValue(WidgetClassToSpawn, PropertyForPin, DefaultValueAsString))
			{
				if (WidgetClassToSpawn->ClassDefaultObject)
				{
					FBlueprintEditorUtils::PropertyValueToString(PropertyForPin, (uint8*)WidgetClassToSpawn->ClassDefaultObject.Get(), DefaultValueAsString);
				}
			}

			// First check the string representation of the default value
			if (Schema->DoesDefaultValueMatch(*Pin, DefaultValueAsString))
			{
				DisplayNoteForPin(Pin);
				continue;
			}

			FString UseDefaultValue;
			TObjectPtr<UObject> UseDefaultObject = nullptr;
			FText UseDefaultText;
			constexpr bool bPreserveTextIdentity = true;

			// Next check if the converted default value would be the same to handle cases like None for object pointers
			Schema->GetPinDefaultValuesFromString(Pin->PinType, Pin->GetOwningNodeUnchecked(), DefaultValueAsString, UseDefaultValue, UseDefaultObject, UseDefaultText, bPreserveTextIdentity);
			if (Pin->DefaultValue.Equals(UseDefaultValue, ESearchCase::CaseSensitive) && Pin->DefaultObject == UseDefaultObject && Pin->DefaultTextValue.IdenticalTo(UseDefaultText))
			{
				DisplayNoteForPin(Pin);
				continue;
			}
		}
	}
}

bool UK2Node_CowCreateWidgetAsync::ValidateSpawnVarPinsNameConflicts(const FKismetCompilerContext& CompilerContext) const
{
	// If any exposed pins has conflicting name with internal pins it will cause problems
	// Pins won't be shown
	// Generation of assignments will be wrong
	auto InternalPins = GetInternalPinNames();
	check(WidgetClassToSpawn);

	for (TFieldIterator<FProperty> PropertyIt(WidgetClassToSpawn, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
		const bool bIsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if(	bIsExposedToSpawn &&
			!Property->HasAnyPropertyFlags(CPF_Parm) && 
			bIsSettableExternally &&
			Property->HasAllPropertyFlags(CPF_BlueprintVisible) &&
			!bIsDelegate &&
			FBlueprintEditorUtils::PropertyStillExists(Property))
		{
			int32 Index = InternalPins.Find(Property->GetFName());
			if (const bool bPropertyNameIsConflictingWithInternalPins = Index != INDEX_NONE)
			{
				CompilerContext.MessageLog.Error(*FString::Printf(TEXT("@@ property %s from type %s has conflicting name with internal pin %s"),
					*Property->GetName(), *GetNameSafe(WidgetClassToSpawn), *InternalPins[Index].ToString()), this);
				return false;
			}
		}
	}

	return true;
}

FText UK2Node_CowCreateWidgetAsync::GetBaseNodeTitle() const
{
	return LOCTEXT("CreateWidget", "Cow Create Widget Async");
}

FText UK2Node_CowCreateWidgetAsync::GetNodeTitleFormat() const
{
	return LOCTEXT("CreateWidget", "Cow Create {ClassName} Widget Async");
}

void UK2Node_CowCreateWidgetAsync::TryCreateOnWidgetCreatedPin()
{
	if (FindPin(WidgetCreated, EGPD_Output) == nullptr)
	{
		FEdGraphPinType PinType;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Exec;
		CreatePin(EGPD_Output, PinType, WidgetCreated, 2);
	}
}

void UK2Node_CowCreateWidgetAsync::PostReconstructNode()
{
	Super::PostReconstructNode();

	// When node is refreshed node's lifecycle doesn't call any events we defined to fixup the state
    // So we need also to handle it separately in here
	OnSoftWidgetClassChanged();
}

void UK2Node_CowCreateWidgetAsync::PostLoad()
{
	// Regenerate pins because
	// We could have changed dependant class while blueprint with the node was closed, therefore we missed OnChanged event
	if (HasValidBlueprint())
	{
		OnSoftWidgetClassChanged();
	}
	
	Super::PostLoad();
}

void UK2Node_CowCreateWidgetAsync::BeginDestroy()
{
	UnbindFromBlueprintChange();
	
	Super::BeginDestroy();
}

void UK2Node_CowCreateWidgetAsync::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Hide "Class" pin from UK2Node_CreateWidget
	// It is used to reuse parent functionality to spawn ExposedOnSpawn pins from selected class
	UEdGraphPin* ClassPin = FindPin(WidgetClass, EGPD_Input);
	check(ClassPin);
	ClassPin->bHidden = true;

	FEdGraphPinType ExtensionVariableType;
	ExtensionVariableType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
	ExtensionVariableType.PinSubCategoryObject = UUserWidget::StaticClass();
	CreatePin(EGPD_Input, ExtensionVariableType, SoftWidgetClass);

	TryCreateOnWidgetCreatedPin();
}

void UK2Node_CowCreateWidgetAsync::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	// On purpose omitted Super::ExpandNode because it will duplicate a lot of ongoing logic
	UK2Node_ConstructObjectFromClass::ExpandNode(CompilerContext, SourceGraph);
	
	if (WidgetClassToSpawn == nullptr)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CowCreateWidgetAsync_Error", "Create Widget Async node @@ must have a class specified.").ToString(), this);
		// we break exec links so this is the only error we get, don't want the CreateWidget node being considered and giving 'unexpected node' type warnings
		BreakAllNodeLinks();
		return;
	}

	if (!ValidateSpawnVarPinsNameConflicts(CompilerContext))
	{
		BreakAllNodeLinks();
		return;
	}
	
    // Graph for better understanding implementation details:
    //
    //             /> Then (Executed immediately after LoadAsset call)
    // LoadAsset -|
    //             \> OnCompleted -> Cast to UUserWidget class -> Call UWidgetBlueprintLibrary::Create -> Generate assignments via SetPropertyByName -> OnWidgetCompleted pin
    //                             (because return is Object)
    //
    // Create intermediate nodes:
	//	1. LoadAsset node
	//  2. CallFunction to UKismetSystemLibrary::Conv_ObjectToClass
	//  3. CallFunction to UWidgetBlueprintLibrary::Create (taken from parent class)
    //  4. Spawn bunch of assignments via SetPropertyByName
    //  5. Hook last assignment to the OnWidgetCompleted pin 
	//
	// Break all pins to this node because it was fake anything is passed via intermediate nodes
	
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	
	UK2Node_LoadAsset* LoadAsset = CompilerContext.SpawnIntermediateNode<UK2Node_LoadAsset>(this, SourceGraph);
	LoadAsset->AllocateDefaultPins();
	UEdGraphPin* LoadAsset_InputAsset = LoadAsset->FindPinChecked(LoadAsset_Input);
	UEdGraphPin* LoadAsset_OutputObject = LoadAsset->FindPinChecked(LoadAsset_Output);
	UEdGraphPin* LoadAsset_OutputCompleted = LoadAsset->FindPinChecked(UEdGraphSchema_K2::PN_Completed, EGPD_Output);

	UK2Node_CallFunction* Cast_ObjectToClass = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	Cast_ObjectToClass->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_ObjectToClass), UKismetSystemLibrary::StaticClass());
	Cast_ObjectToClass->AllocateDefaultPins();
	UEdGraphPin* Cast_InputObject = Cast_ObjectToClass->FindPinChecked(FName(TEXT("Object")), EGPD_Input);
	UEdGraphPin* Cast_InputClass  = Cast_ObjectToClass->FindPinChecked(FName(TEXT("Class")), EGPD_Input);
	UEdGraphPin* Cast_ReturnValue = Cast_ObjectToClass->GetReturnValuePin();
	
	UEdGraphPin* This_InputSoftRef = GetSoftWidgetPin();
	UEdGraphPin* This_OutputOnWidgetCreated = FindPinChecked(WidgetCreated, EGPD_Output);

	// 1. this.exec to LoadAsset.exec
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *LoadAsset->GetExecPin());
	// 2. this.then to LoadAsset.then
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *LoadAsset->GetThenPin());

	// 3. this.SoftWidgetClass to LoadAsset.Asset
    // We can select SoftWidgetClass to spawn in 2 ways
    //
    // a. We can connect some other pin to SoftWidgetClass pin
    //      In such a case we need to do an additional conversation SoftClass -> SoftClassPath -> SoftObjectReference
    //      Otherwise we cannot connect it to LoadAsset_Input
    if (!This_InputSoftRef->LinkedTo.IsEmpty())
    {
	    // Notify user if he introduced hard-ref through the linked pin
    	UEdGraphPin* SoftWidgetSourcePin = This_InputSoftRef->LinkedTo[0];
    	if (UClass* SourceClass = Cast<UClass>(SoftWidgetSourcePin->PinType.PinSubCategoryObject.Get()))
    	{
    		if (!SourceClass->HasAnyClassFlags(CLASS_Native))
    		{
    			// Let's be honest - I won't localize it, so fuck this LOCTEXT macro :)
    			CompilerContext.MessageLog.Note(*FString::Printf(TEXT("You introduced hard-ref to %s via @@ pin. Make sure it doesn't happened accidentally and you actually wanted it"), *SourceClass->GetName()), SoftWidgetSourcePin);
    		}
    	}
    	
        UEdGraphPin* ConversationReturnPin = GenerateConvertToSoftObjectRef(CompilerContext, SourceGraph, This_InputSoftRef);
        ensureAlways(Schema->TryCreateConnection(ConversationReturnPin, LoadAsset_InputAsset));
    	
    	// If we happen to connect SoftWidgetClass via Link to another pin (meaning at runtime the class may be different)
    	// Give a note for any default values that aren't changed from the base class
    	ValidateSpawnVarPins(CompilerContext);
    }
    // b. We selected class directly using drop-down class picker
    //      The selected class is in Pin->DefaultValue and we can simply transfer the data
    else
    {
        CompilerContext.MovePinLinksToIntermediate(*This_InputSoftRef, *LoadAsset_InputAsset);
    }

	// 4. LoadAsset.Object to Cast.Object
	Cast_InputClass->DefaultObject = UUserWidget::StaticClass();
	ensureAlways(Schema->TryCreateConnection(LoadAsset_OutputObject, Cast_InputObject));

	UEdGraphPin* This_InputWorldContextPin = GetWorldContextPin();
	UEdGraphPin* This_OwningPlayerPin = GetOwningPlayerPin();
	UEdGraphPin* This_NodeResult = GetResultPin();
	
	//////////////////////////////////////////////////////////////////////////
	// create 'UWidgetBlueprintLibrary::Create' call node
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create), UWidgetBlueprintLibrary::StaticClass());
	CallCreateNode->AllocateDefaultPins();
	
	UEdGraphPin* Create_InputExec = CallCreateNode->GetExecPin();
	UEdGraphPin* Create_InputWorldContextPin = CallCreateNode->FindPinChecked(Create_InputWorldContextObject);
	UEdGraphPin* Create_InputWidgetTypePin = CallCreateNode->FindPinChecked(Create_InputWidgetType);
	UEdGraphPin* Create_InputOwningPlayerPin = CallCreateNode->FindPinChecked(Create_InputOwningPlayer);
	UEdGraphPin* Create_OutputResult = CallCreateNode->GetReturnValuePin();

	// move this.completed to 'UWidgetBlueprintLibrary::Create'
	ensureAlways(Schema->TryCreateConnection(LoadAsset_OutputCompleted, Create_InputExec));

	//  5. Cast.ReturnValue to Create.WidgetType
	Cast_ReturnValue->PinType = Create_InputWidgetTypePin->PinType; // (Type match required to connect pins)
	ensureAlways(Schema->TryCreateConnection(Cast_ReturnValue, Create_InputWidgetTypePin));

	// Copy the world context connection from the spawn node to 'UWidgetBlueprintLibrary::Create' if necessary
	if ( This_InputWorldContextPin )
	{
		CompilerContext.MovePinLinksToIntermediate(*This_InputWorldContextPin, *Create_InputWorldContextPin);
	}

	// Copy the 'Owning Player' connection from the spawn node to 'UWidgetBlueprintLibrary::Create'
	CompilerContext.MovePinLinksToIntermediate(*This_OwningPlayerPin, *Create_InputOwningPlayerPin);

	// Move result connection from spawn node to 'UWidgetBlueprintLibrary::Create'
	CompilerContext.MovePinLinksToIntermediate(*This_NodeResult, *Create_OutputResult);

	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes
	UEdGraphPin* LastThen = FCowCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, Create_OutputResult, WidgetClassToSpawn, Create_InputWidgetTypePin);
	
	// Move 'then' connection from create widget node to the last 'then'
	CompilerContext.MovePinLinksToIntermediate(*This_OutputOnWidgetCreated, *LastThen);
	
	BreakAllNodeLinks();
}

FText UK2Node_CowCreateWidgetAsync::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (WidgetClassToSpawn != nullptr)
	{
		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ClassName"), WidgetClassToSpawn->GetDisplayNameText());
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(GetNodeTitleFormat(), Args), this);
		}
		return CachedNodeTitle;
	}
	
	return Super::GetNodeTitle(TitleType);
}

FName UK2Node_CowCreateWidgetAsync::GetCornerIcon() const
{
	return TEXT("Graph.Latent.LatentIcon");
}

bool UK2Node_CowCreateWidgetAsync::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	bool bIsCompatible = false;
	// Can only place events in ubergraphs and macros (other code will help prevent macros with latents from ending up in functions)
	EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
	if (GraphType == EGraphType::GT_Ubergraph || GraphType == EGraphType::GT_Macro)
	{
		bIsCompatible = true;
	}
	return bIsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UK2Node_CowCreateWidgetAsync::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*>* OutClassPins)
{
	// Prevent Super::CreatePinsForClass to change return type because it will introduce hard-ref to widget class
	
	// Cache PinSubCategoryObject because it will be changed by Super and we don't want it
	UEdGraphPin* ResultPin = GetResultPin();
	auto CachedPinSubCategoryObject = ResultPin->PinType.PinSubCategoryObject;
	
	Super::CreatePinsForClass(InClass, OutClassPins);

	ResultPin->PinType.PinSubCategoryObject = CachedPinSubCategoryObject;
}

void UK2Node_CowCreateWidgetAsync::PinDefaultValueChanged(UEdGraphPin* ChangedPin)
{
	if (ChangedPin && (ChangedPin->PinName == SoftWidgetClass))
	{
		OnSoftWidgetClassChanged();
	}

	Super::PinDefaultValueChanged(ChangedPin);
}

void UK2Node_CowCreateWidgetAsync::PinConnectionListChanged(UEdGraphPin* ChangedPin)
{
    if (ChangedPin)
    {
        if (ChangedPin->PinName == SoftWidgetClass)
        {
            OnSoftWidgetClassChanged();
        } else if (ChangedPin == GetResultPin())
        {
            // If we connected Result (Return) pin to anything and it's the only connection
            // We want to make "proper" auto-wire
            // Which will connect node's OnWidgetCreated with the node we hooked Result in.
            //
            // Also see UK2Node_CowCreateWidgetAsync::IsConnectionDisallowed for details how we prevent newly placed node from auto-wiring

            UEdGraphPin* OnWidgetCreated = FindPinChecked(WidgetCreated, EGPD_Output);
            
            const bool bResultHasSingleConnection = GetResultPin()->LinkedTo.Num() == 1;
            const bool bOnWidgetCreatedPinIsntConnected = OnWidgetCreated->LinkedTo.IsEmpty();
            if (bOnWidgetCreatedPinIsntConnected && bResultHasSingleConnection)
            {
                const UEdGraphNode* ResultConnectedNode = GetResultPin()->LinkedTo[0]->GetOwningNode();
                UEdGraphPin* FirstAvailableExec = nullptr;
                const bool bHasConnectedExecs = IsAnyInputExecPinsConnected(ResultConnectedNode->Pins, FirstAvailableExec);
                
                // If the node doesn't have anything connected to execs and has available spot -> connect to it
                if (!bHasConnectedExecs && FirstAvailableExec)
                {
                    const UEdGraphSchema_K2* K2Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());
                    K2Schema->TryCreateConnection(OnWidgetCreated, FirstAvailableExec);
                }
            }
        }
    }

	Super::PinConnectionListChanged(ChangedPin);
}

void UK2Node_CowCreateWidgetAsync::OnSoftWidgetClassChanged()
{
	UEdGraphPin* WidgetClassPin = FindPin(WidgetClass, EGPD_Input);

	/**
	 * When we change (or just check) selected WidgetClass we need to do several things:
	 *  1. Unbind from previous Blueprint's OnChange event
	 *  2. Bind to a new Blueprint's OnChange event (to update exposed pins when they added)
	 *  3. Update Super::WidgetClassPin for Super to correctly populate exposed pins
	 */
	UClass* OldWidgetClass = WidgetClassToSpawn;

    UClass* NewWidgetClass = GetClassToSpawn();

	// If we happen to start the editor but our node has Serialized WidgetClassToSpawn we need to bind to owning OnChanged 
	const bool bBindedToOnChanged = OnWidgetBlueprintChangedHandle.IsValid();
	const bool bRunForOnChangedBinding = NewWidgetClass && !bBindedToOnChanged;
	if (bRunForOnChangedBinding || NewWidgetClass != OldWidgetClass)
	{
		// 1. Unbind from previous Blueprint's OnChange event
		UnbindFromBlueprintChange();

		// 2. Bind to a new Blueprint's OnChange event (to update exposed pins when they added)
		if (NewWidgetClass)
		{
			if (auto BlueprintClass = Cast<UBlueprintGeneratedClass>(NewWidgetClass))
			{
				if (auto Blueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy))
				{
					WeakWidgetClassBlueprint = Blueprint;
					OnWidgetBlueprintChangedHandle = Blueprint->OnChanged().AddWeakLambda(this, [this](class UBlueprint*)
					{
						OnSoftWidgetClassChanged();
					});			
				}
			}
		}

		WidgetClassToSpawn = NewWidgetClass;
	}

    // Fix our return type
    //
    // If soft widget class pin connected to anything that means that our type is propagated
    // And it's safe to use it (because hard-ref to the type is already created by the connection)
    // We're not the one to blame in such a situation :)
    UEdGraphPin* ResultPin = GetResultPin();
    if (IsSoftWidgetClassConnected())
    {
        ResultPin->PinType.PinSubCategoryObject = WidgetClassToSpawn;
    } else
    {
        // To not introduce hard-ref we need to iterate over Super classes until we find a most derived C++ base

        // Example
        //
        // UProjectWidget : UUserWidget
        // WBP_Base : UProjectWidget
        // WBP_Derived : WBP_Base
        //
        // If user selected to spawn WBP_Derived we need to set return to be UProjectWidget
        ResultPin->PinType.PinSubCategoryObject = GetFirstNativeClass(WidgetClassToSpawn);
    }

	// 3. Update Super::WidgetClassPin for Super to correctly populate exposed pins
	WidgetClassPin->DefaultObject = WidgetClassToSpawn;

	// Always try to regenerate 
	OnClassPinChanged();
	
	// Reset Parent Class pin to avoid hard-ref
	WidgetClassPin->DefaultObject = nullptr;
	
	// OnClassPinChanged removes OnWidgetCreatedPin
	TryCreateOnWidgetCreatedPin();
}

void UK2Node_CowCreateWidgetAsync::UnbindFromBlueprintChange()
{
	if (auto Blueprint = WeakWidgetClassBlueprint.Get())
	{
		Blueprint->OnChanged().Remove(OnWidgetBlueprintChangedHandle);
		OnWidgetBlueprintChangedHandle.Reset();
		WeakWidgetClassBlueprint.Reset();
	}
}

bool UK2Node_CowCreateWidgetAsync::IsSpawnVarPin(UEdGraphPin* Pin) const
{
	return( Super::IsSpawnVarPin(Pin) &&
		Pin->PinName != SoftWidgetClass &&
		Pin->PinName != WidgetCreated);
}

bool UK2Node_CowCreateWidgetAsync::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
    // Fix auto-wiring uses Then instead of OnWidgetCreated when dragging and using Result (UUserWidget object)

    // Idea is to detect when newly placed node does Autowire and tries to connect our "Then" pin to the node's "Exec"
    if (MyPin == GetThenPin())
    {
        // Return is connected
        if (const bool bResultConnected = GetResultPin()->LinkedTo.Num() == 1)
        {
            // We need to make sure we're currently being called to check possibility of connection between the node to which our return is connected
            UEdGraphNode* ReturnLinkedNode = GetResultPin()->LinkedTo[0]->GetOwningNode();
            if (const bool bReturnLinkedToOtherPinNode = ReturnLinkedNode == OtherPin->GetOwningNode())
            {
                UEdGraphPin* NotUsed = nullptr;
                // We disallow connection in a case when node's execs aren't connected which means that we are in that exact moment trying to do auto-wire
                // And we want to prevent it for UK2Node_CowCreateWidgetAsync::PinConnectionListChanged to do the correct auto-wire we want
                // It's alright if you don't understand it, I struggled 10 minutes to write the comment of that single line...
                return IsAnyInputExecPinsConnected(ReturnLinkedNode->Pins, NotUsed);
            }
        }

    }

    return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

bool UK2Node_CowCreateWidgetAsync::IsAnyInputExecPinsConnected(const TArray<UEdGraphPin*>& Pins, UEdGraphPin*& OutFirstUnconnectedPin)
{
    OutFirstUnconnectedPin = nullptr;

    for (UEdGraphPin* Pin : Pins)
    {
        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EEdGraphPinDirection::EGPD_Input)
        {
            if (!Pin->LinkedTo.IsEmpty())
            {
                return true;
            }
            if (OutFirstUnconnectedPin == nullptr)
            {
                OutFirstUnconnectedPin = Pin;
            }
        }
    }

    return false;
}

UClass* UK2Node_CowCreateWidgetAsync::GetClassToSpawn() const
{
    UEdGraphPin* SoftWidgetClassPin = FindPinChecked(SoftWidgetClass, EGPD_Input);

    // If SoftWidgetClassPin isn't connected to anything and not empty we should use Path written in DefaultValue
	if (!SoftWidgetClassPin->DefaultValue.IsEmpty() && SoftWidgetClassPin->LinkedTo.Num() == 0)
	{
        FSoftObjectPath SoftWidget = SoftWidgetClassPin->DefaultValue;
        return Cast<UClass>(SoftWidget.TryLoad());
	}
	else if (SoftWidgetClassPin->LinkedTo.Num())
	{
		UEdGraphPin* ClassSource = SoftWidgetClassPin->LinkedTo[0];
		return Cast<UClass>(ClassSource->PinType.PinSubCategoryObject.Get());
	}
    
    return nullptr;
}

UEdGraphPin* UK2Node_CowCreateWidgetAsync::GenerateConvertToSoftObjectRef(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* SoftClassPin)
{
    if (!ensureAlways(SoftClassPin && SoftClassPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass))
    {
        return nullptr;
    }

    //
    // SoftClassPin -> Conv_SoftObjRefToSoftClassPath -> Conv_SoftObjPathToSoftObjRef (And return "return" pin of last conv)
    //

    const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

    // FSoftClassPath UKismetSystemLibrary::Conv_SoftObjRefToSoftClassPath(TSoftClassPtr<UObject> SoftClassReference)
    UK2Node_CallFunction* SoftObjRefToSoftClassPath = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    SoftObjRefToSoftClassPath->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjRefToSoftClassPath)));
	SoftObjRefToSoftClassPath->AllocateDefaultPins();

    UEdGraphPin* SoftToPath_Input = SoftObjRefToSoftClassPath->FindPinChecked(TEXT("SoftClassReference"), EGPD_Input);
    UEdGraphPin* SoftToPath_ReturnValue = SoftObjRefToSoftClassPath->GetReturnValuePin();

    CompilerContext.MovePinLinksToIntermediate(*SoftClassPin, *SoftToPath_Input);

    // TSoftObjectPtr<UObject> UKismetSystemLibrary::Conv_SoftObjPathToSoftObjRef(const FSoftObjectPath& SoftObjectPath)
    UK2Node_CallFunction* SoftObjPathToSoftObjRef = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    SoftObjPathToSoftObjRef->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef)));
	SoftObjPathToSoftObjRef->AllocateDefaultPins();

    UEdGraphPin* PathToRef_Input = SoftObjPathToSoftObjRef->FindPinChecked(TEXT("SoftObjectPath"), EGPD_Input);
    ensureAlways(Schema->TryCreateConnection(SoftToPath_ReturnValue, PathToRef_Input));
    
    return SoftObjPathToSoftObjRef->GetReturnValuePin();
}

bool UK2Node_CowCreateWidgetAsync::IsSoftWidgetClassConnected() const
{
    UEdGraphPin* SoftWidgetClassPin = FindPinChecked(SoftWidgetClass, EGPD_Input);
    return !SoftWidgetClassPin->LinkedTo.IsEmpty();
}

UClass* UK2Node_CowCreateWidgetAsync::GetFirstNativeClass(UClass* Child)
{
    UClass* Result = Child;
    while (Result && !Result->HasAnyClassFlags(CLASS_Native))
    {
        Result = Result->GetSuperClass();
    }
    return Result;
}

#undef LOCTEXT_NAMESPACE