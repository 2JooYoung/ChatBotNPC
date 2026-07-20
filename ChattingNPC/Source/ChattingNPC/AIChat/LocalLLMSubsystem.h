// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "AIChat/NPCChatTypes.h"
#include "AIChat/NPCDialogueContext.h"
#include "LocalLLMSubsystem.generated.h"

class UNPCProfileDataAsset;
class IConsoleObject;

/** Fired on the game thread when an NPC response is successfully generated. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNPCResponseReceived, FName, NPCId, const FString&, Response);

/** Fired on the game thread when a request fails. StatusCode is the HTTP code (0 for internal errors). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNPCRequestFailed, FName, NPCId, const FString&, UserMessage, int32, StatusCode);

/**
 * Owns all local-LLM communication and per-NPC conversation history.
 * Handles JSON (de)serialization, system-prompt assembly, request de-duplication,
 * timeouts, and discarding late responses after a conversation ends.
 *
 * Generates dialogue only. Never mutates game state from a model response.
 */
UCLASS()
class CHATTINGNPC_API ULocalLLMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Broadcast on success (NPCId, response text). */
	UPROPERTY(BlueprintAssignable, Category = "Local LLM")
	FOnNPCResponseReceived OnResponseReceived;

	/** Broadcast on failure (NPCId, user-facing message, status code). */
	UPROPERTY(BlueprintAssignable, Category = "Local LLM")
	FOnNPCRequestFailed OnRequestFailed;

	/**
	 * Send a player message to the given NPC's LLM.
	 * Returns false and broadcasts OnRequestFailed immediately for invalid input
	 * (null profile, empty message, or a request already in flight for this NPC).
	 */
	UFUNCTION(BlueprintCallable, Category = "Local LLM")
	bool SendMessageToNPC(UNPCProfileDataAsset* Profile, const FString& PlayerMessage);

	/** Replace the shared game dialogue context injected into system prompts. */
	UFUNCTION(BlueprintCallable, Category = "Local LLM")
	void SetDialogueContext(const FNPCDialogueContext& InContext) { DialogueContext = InContext; }

	/** Clear a single NPC's history and invalidate any in-flight request for it. */
	UFUNCTION(BlueprintCallable, Category = "Local LLM")
	void ClearConversation(FName NPCId);

	/** Clear all NPC histories. */
	UFUNCTION(BlueprintCallable, Category = "Local LLM")
	void ClearAllConversations();

	/**
	 * Invalidate the in-flight request for an NPC without clearing history.
	 * A late-arriving response is then discarded. Call this when a conversation ends.
	 */
	UFUNCTION(BlueprintCallable, Category = "Local LLM")
	void CancelRequest(FName NPCId);

	UFUNCTION(BlueprintPure, Category = "Local LLM")
	bool IsRequestInFlight(FName NPCId) const { return InFlightRequests.Contains(NPCId); }

	/** Read-only access to a stored session (empty if none). */
	const FNPCConversationSession* FindSession(FName NPCId) const { return Sessions.Find(NPCId); }

	/** Phase 2 test helper: sends a fixed message with a transient test profile. */
	UFUNCTION(BlueprintCallable, Category = "Local LLM|Debug")
	void SendTestMessage();

	/** Logs every NPC's stored history (count + each turn) for in-game verification. */
	UFUNCTION(BlueprintCallable, Category = "Local LLM|Debug")
	void DumpConversationHistory();

private:
	FNPCConversationSession& GetOrCreateSession(FName NPCId);

	void HandleResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully,
		FName NPCId, uint64 RequestId, FString PlayerMessage);

	static bool ParseResponseContent(const FString& Body, FString& OutContent);

	//~ Debug delegate handlers (log only)
	UFUNCTION()
	void HandleDebugResponse(FName NPCId, const FString& Response);

	UFUNCTION()
	void HandleDebugFailure(FName NPCId, const FString& UserMessage, int32 StatusCode);

	/** Shared context (test defaults until wired to real systems). */
	FNPCDialogueContext DialogueContext;

	/** Per-NPC stored user/assistant history. */
	TMap<FName, FNPCConversationSession> Sessions;

	/** NPCs with a request currently in flight (prevents duplicate concurrent sends). */
	TSet<FName> InFlightRequests;

	/** NPCId -> currently valid request id. A callback whose id no longer matches is stale. */
	TMap<FName, uint64> ActiveRequestIds;

	uint64 NextRequestId = 1;

	/** Keeps the transient test profile alive across the async call. */
	UPROPERTY()
	TObjectPtr<UNPCProfileDataAsset> TestProfile;

	IConsoleObject* TestConsoleCommand = nullptr;
	IConsoleObject* HistoryConsoleCommand = nullptr;
};
