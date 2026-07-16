// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NPCDialogueContext.generated.h"

/**
 * Loosely-coupled game context injected into the system prompt.
 * Uses test defaults for now; later wire this to real quest/inventory systems
 * without touching the LLM communication code.
 */
USTRUCT(BlueprintType)
struct FNPCDialogueContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialogue")
	FString PlayerName = TEXT("플레이어");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialogue")
	FString CurrentLocation = TEXT("시작 마을");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialogue")
	FString CurrentQuestState = TEXT("진행 중인 퀘스트 없음");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialogue")
	TArray<FString> PlayerOwnedItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Dialogue")
	FString TimeOfDay = TEXT("낮");
};
