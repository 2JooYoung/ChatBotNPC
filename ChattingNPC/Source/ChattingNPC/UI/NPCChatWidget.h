// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPCChatWidget.generated.h"

class ANPCCharacter;
class UNPCProfileDataAsset;

/** Fired when the widget's close button is pressed. The owner orchestrates teardown. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChatCloseRequested);

/**
 * C++ base for the NPC chat UI. Build the layout in WBP_NPCChat (derive from this).
 * Owns the send/response flow and subscribes to ULocalLLMSubsystem; the Blueprint
 * only wires buttons/text to the exposed functions and implements the visual events.
 */
UCLASS(Abstract)
class CHATTINGNPC_API UNPCChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Owner (player) binds this to be notified when the close button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "NPC Chat")
	FOnChatCloseRequested OnCloseRequested;

	/** Begin a conversation: set NPC, subscribe to the LLM, show the greeting. */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void StartConversation(ANPCCharacter* NPC);

	/** Set the active NPC (and cache its profile/id) without other side effects. */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void SetCurrentNPC(ANPCCharacter* NPC);

	/** Validate and send the player's message to the local LLM. */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void SendPlayerMessage(const FText& Message);

	/** WBP close button calls this; it only requests close via OnCloseRequested. */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void EndConversation();

	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void SetWaitingForResponse(bool bWaiting);

	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void AddPlayerMessage(const FText& Message);

	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void AddNPCMessage(const FText& Message);

	UFUNCTION(BlueprintCallable, Category = "NPC Chat")
	void ShowErrorMessage(const FText& ErrorMessage);

	UFUNCTION(BlueprintPure, Category = "NPC Chat")
	bool IsWaitingForResponse() const { return bWaitingForResponse; }

	UFUNCTION(BlueprintPure, Category = "NPC Chat")
	ANPCCharacter* GetCurrentNPC() const { return CurrentNPC.Get(); }

	/** Called by the owner during teardown: unsubscribe, cancel request, fire ended. */
	void HandleConversationShutdown();

protected:
	virtual void NativeDestruct() override;

	//~ Visual events implemented in the WBP.
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnConversationStarted(const FText& NPCName, const FText& NPCRole);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnPlayerMessageAdded(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnNPCMessageAdded(const FText& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnRequestStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnRequestCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnRequestFailed(const FText& ErrorMessage, int32 StatusCode);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnConversationEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnWaitingForResponseChanged(bool bWaiting);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Chat")
	void OnErrorMessage(const FText& ErrorMessage);

private:
	//~ Subsystem delegate handlers (must be UFUNCTION for AddDynamic).
	UFUNCTION()
	void HandleLLMResponse(FName NPCId, const FString& Response);

	UFUNCTION()
	void HandleLLMFailure(FName NPCId, const FString& UserMessage, int32 StatusCode);

	class ULocalLLMSubsystem* GetLLM() const;
	void SubscribeToLLM();
	void UnsubscribeFromLLM();

	TWeakObjectPtr<ANPCCharacter> CurrentNPC;
	TWeakObjectPtr<UNPCProfileDataAsset> CurrentProfile;
	FName CurrentNPCId;

	bool bWaitingForResponse = false;
	bool bSubscribed = false;
};
