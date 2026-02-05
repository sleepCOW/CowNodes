// Copyright (c) 2025 Oleksandr "sleepCOW" Ozerov. All rights reserved.

using UnrealBuildTool;

public class CowNodes : ModuleRules
{
	public CowNodes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"UnrealEd",
				"UMG",
				"Slate",
				"SlateCore",
				"UMGEditor",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"BlueprintGraph",
				"Kismet",
				"KismetCompiler",
				"AssetRegistry",
				"CowRuntime"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
