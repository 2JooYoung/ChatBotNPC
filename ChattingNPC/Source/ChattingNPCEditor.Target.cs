// Copyright ChattingNPC. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ChattingNPCEditorTarget : TargetRules
{
	public ChattingNPCEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ChattingNPC");
	}
}
