// Copyright ChattingNPC. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ChattingNPCTarget : TargetRules
{
	public ChattingNPCTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ChattingNPC");
	}
}
