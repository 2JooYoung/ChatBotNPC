// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NPCChatTypes.generated.h"

/** OpenAI-compatible chat roles used by llama-server. */
namespace NPCChatRoles
{
	inline const FString System = TEXT("system");
	inline const FString User = TEXT("user");
	inline const FString Assistant = TEXT("assistant");
}

/**
 * A single chat message in the OpenAI-compatible messages array.
 * Role is one of: "system", "user", "assistant".
 */
USTRUCT(BlueprintType)
struct FNPCChatMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Chat")
	FString Role;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC Chat")
	FString Content;

	FNPCChatMessage() = default;

	FNPCChatMessage(const FString& InRole, const FString& InContent)
		: Role(InRole)
		, Content(InContent)
	{
	}
};

/**
 * Per-NPC conversation history. Stores only user/assistant turns;
 * the system prompt is rebuilt fresh for every request and never stored here.
 */
USTRUCT(BlueprintType)
struct FNPCConversationSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "NPC Chat")
	FName NPCId;

	UPROPERTY(BlueprintReadOnly, Category = "NPC Chat")
	TArray<FNPCChatMessage> Messages;

	/** Append a message (Role should be user/assistant for stored history). */
	void AddMessage(const FString& Role, const FString& Content)
	{
		Messages.Emplace(Role, Content);
	}

	/**
	 * Keep only the most recent MaxMessages entries.
	 * MaxMessages <= 0 disables trimming.
	 */
	void TrimHistory(int32 MaxMessages)
	{
		if (MaxMessages <= 0)
		{
			return;
		}

		const int32 Overflow = Messages.Num() - MaxMessages;
		if (Overflow > 0)
		{
			Messages.RemoveAt(0, Overflow, EAllowShrinking::No);
		}
	}

	void Clear()
	{
		Messages.Reset();
	}
};
