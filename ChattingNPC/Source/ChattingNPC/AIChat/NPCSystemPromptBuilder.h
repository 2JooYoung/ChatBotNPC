// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UNPCProfileDataAsset;
struct FNPCDialogueContext;

/**
 * Pure helper that turns an NPC profile + game context into a system prompt.
 * Kept free of gameplay/UObject state so it can be unit-reasoned in isolation.
 */
class CHATTINGNPC_API FNPCSystemPromptBuilder
{
public:
	static FString Build(const UNPCProfileDataAsset* Profile, const FNPCDialogueContext& Context);
};
