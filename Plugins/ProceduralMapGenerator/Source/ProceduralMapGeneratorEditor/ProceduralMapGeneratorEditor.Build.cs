// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProceduralMapGeneratorEditor : ModuleRules
{
	public ProceduralMapGeneratorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"UnrealEd",
			"LevelEditor",
			"PropertyEditor",
			"AssetRegistry",
			"ContentBrowser",
			"WorkspaceMenuStructure",
			"EditorSubsystem",
			"Projects"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ProceduralMapGenerator",
			"AssetTools",
			"Json",
			"JsonUtilities"
		});
	}
}
