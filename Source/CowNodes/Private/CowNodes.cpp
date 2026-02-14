// Copyright (c) 2026 Oleksandr "sleepCOW" Ozerov. All rights reserved.

#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "K2Node_CallFunction.h"

class FCowNodesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FCowNodesModule::RegisterMenuExtensions));
	}

	virtual void ShutdownModule() override
	{
	}

	static void CreateActionNodeSection(UToolMenu* NodeMenu)
	{
		// Create menu to control pins visibility on nodes
		UGraphNodeContextMenuContext* NodeContext = NodeMenu->FindContext<UGraphNodeContextMenuContext>();
		if (!NodeContext)
		{
			return;
		}

		if (!NodeContext->Node)
		{
			return;
		}
		const UEdGraphNode* Node = NodeContext->Node;

		FToolMenuSection& CowActionsSection = NodeMenu->AddSection(TEXT("CowActions"), INVTEXT("Cow Actions"), FToolMenuInsert(TEXT("EdGraphSchemaNodeActions"), EToolMenuInsertType::After));

		auto SubMenuCreate = FNewToolMenuDelegate::CreateLambda([Node](UToolMenu* PinVisibilityMenu)
		{
			FToolMenuSection& PinVisibilitySection = PinVisibilityMenu->AddSection("CowPinVisibilitySection");

			for (UEdGraphPin* Pin : Node->GetAllPins())
			{
				if (Pin && Pin->Direction == EGPD_Input)
				{
					PinVisibilitySection.AddEntry(FToolMenuEntry::InitMenuEntry
					(
						Pin->PinName,
						FText::FromString(FString::Printf(TEXT("%s"), *Pin->PinName.ToString())),
						INVTEXT("Toggle pin visibility"),
						FSlateIcon(),
						FUIAction
						(
							FExecuteAction::CreateLambda([Pin, Node]() 
														{
															Pin->SafeSetHidden(!Pin->bHidden);
															Node->GetGraph()->NotifyGraphChanged();
														}),
							FCanExecuteAction(),
							FGetActionCheckState::CreateLambda([Pin]() { return Pin->bHidden ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
						),
						EUserInterfaceActionType::ToggleButton
					));
				}
			}
		});

		CowActionsSection.AddSubMenu(TEXT("CowPinVisibility"),
									 INVTEXT("Pins Visibility"),
									 INVTEXT("Control pins visibility might be useful to display WorldContextPin if you want to avoid ShowWorldContextPin meta"),
									 SubMenuCreate);
	}

	static void RegisterMenuExtensions()
	{
		// Add a new section to the Select menu right after the "BSP" section
		UToolMenu* NodeActionsMenu = UToolMenus::Get()->ExtendMenu(TEXT("GraphEditor.GraphNodeContextMenu.K2Node_CallFunction"));
		NodeActionsMenu->AddDynamicSection(TEXT("CowActions"), FNewSectionConstructChoice(FNewToolMenuDelegate::CreateStatic(&FCowNodesModule::CreateActionNodeSection)));
	}
};

IMPLEMENT_MODULE(FCowNodesModule, CowNodes);