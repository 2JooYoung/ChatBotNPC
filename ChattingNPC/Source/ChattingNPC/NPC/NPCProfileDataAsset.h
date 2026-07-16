// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NPCProfileDataAsset.generated.h"

/**
 * Per-NPC personality/knowledge profile.
 * Create one asset per NPC in the editor (e.g. DA_NPC_Blacksmith).
 * All NPCs share the same local LLM but behave differently based on this data.
 */
UCLASS(BlueprintType)
class CHATTINGNPC_API UNPCProfileDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable unique id used as the conversation-history key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Identity")
	FName NPCId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Identity")
	FText NPCName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Identity")
	FText Role;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Persona", meta = (MultiLine = true))
	FString Personality;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Persona", meta = (MultiLine = true))
	FString SpeakingStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Persona", meta = (MultiLine = true))
	FString Background;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Knowledge")
	TArray<FString> KnownInformation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Knowledge")
	TArray<FString> UnknownInformation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Knowledge")
	TArray<FString> ForbiddenTopics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Dialogue", meta = (MultiLine = true))
	FText InitialGreeting;

	/** Per-NPC generation tuning. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Generation", meta = (ClampMin = "1"))
	int32 MaxResponseTokens = 200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Profile|Generation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Temperature = 0.7f;
};
