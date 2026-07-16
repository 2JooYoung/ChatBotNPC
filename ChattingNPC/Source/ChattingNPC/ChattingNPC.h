// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Dedicated log category for the local-LLM NPC chat system (module-internal).
DECLARE_LOG_CATEGORY_EXTERN(LogChattingNPC, Log, All);

/**
 * On-screen debug message helper for PIE verification (no-op in Shipping).
 * Complements UE_LOG so runtime flow is visible without opening the Output Log.
 */
void ChattingNPCScreenLog(const FString& Message, const FColor& Color = FColor::Cyan, float DisplayTime = 4.0f);
