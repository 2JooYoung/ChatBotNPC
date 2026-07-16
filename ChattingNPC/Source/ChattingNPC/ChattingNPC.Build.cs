// Copyright ChattingNPC. All Rights Reserved.

using UnrealBuildTool;

public class ChattingNPC : ModuleRules
{
	public ChattingNPC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// No Public/Private split: expose the module root so folder-relative
		// includes ("AIChat/...", "NPC/...") resolve in both hand-written and
		// UHT-generated code under modern build settings.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP",
			"Json",
			"JsonUtilities",
			"UMG",
			"Slate",
			"SlateCore",
			"DeveloperSettings"
		});
	}
}
