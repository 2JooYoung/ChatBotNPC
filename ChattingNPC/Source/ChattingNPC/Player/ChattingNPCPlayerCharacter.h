// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPC/NPCInteractorInterface.h"
#include "ChattingNPCPlayerCharacter.generated.h"

class UInputAction;
class UInputComponent;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
class ANPCCharacter;
class UNPCChatWidget;
struct FInputActionValue;

/**
 * Self-contained third-person player character with built-in camera, Enhanced Input
 * movement/look/jump, and NPC interaction (IA_Interact, nearest-NPC selection,
 * conversation lifecycle). Create a BP child (e.g. BP_Player), assign the input
 * assets + mesh, and set it as the GameMode Default Pawn.
 *
 * Implements INPCInteractorInterface so NPCs register/unregister themselves as they
 * enter/leave range without any hard dependency on this concrete class.
 */
UCLASS()
class CHATTINGNPC_API AChattingNPCPlayerCharacter : public ACharacter, public INPCInteractorInterface
{
	GENERATED_BODY()

public:
	AChattingNPCPlayerCharacter();

	//~ INPCInteractorInterface
	virtual void NotifyNPCEnteredRange(ANPCCharacter* NPC) override;
	virtual void NotifyNPCExitedRange(ANPCCharacter* NPC) override;

	UFUNCTION(BlueprintPure, Category = "NPC Interaction")
	ANPCCharacter* GetCurrentNPC() const { return CurrentNPC.Get(); }

	UFUNCTION(BlueprintPure, Category = "NPC Interaction")
	bool IsInConversation() const { return bIsInConversation; }

	/** End the active conversation and restore movement/input (safe to call anytime). */
	UFUNCTION(BlueprintCallable, Category = "NPC Interaction")
	void EndConversation();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Standard third-person movement/look handlers. */
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	/** IA_Interact (E) handler: toggles conversation with the nearest NPC. */
	void OnInteract();

	ANPCCharacter* FindNearestNPC() const;
	void StartConversationWith(ANPCCharacter* NPC);

	/** Lock/unlock movement + look, toggle cursor, and switch input mode. */
	void SetConversationInputMode(bool bEnable);

	/** Bound to the chat widget's close button request. */
	UFUNCTION()
	void HandleChatCloseRequested();

	//~ Blueprint hooks for the chat widget (wired in Phase 5).
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Interaction")
	void OnConversationStartedWithNPC(ANPCCharacter* NPC);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC Interaction")
	void OnConversationEndedWithNPC();

	/** Spring arm + follow camera for third-person view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	//~ Input assets — assign in the BP child (BP_Player) defaults.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/** WBP_NPCChat class. Assign in the BP child (BP_Player) defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC Interaction")
	TSubclassOf<UNPCChatWidget> ChatWidgetClass;

private:
	/** Active chat widget instance (created on conversation start). */
	UPROPERTY(Transient)
	TObjectPtr<UNPCChatWidget> ChatWidget;

	/** NPCs currently overlapping this player's interaction. */
	TArray<TWeakObjectPtr<ANPCCharacter>> NPCsInRange;

	/** NPC the player is currently talking to. */
	TWeakObjectPtr<ANPCCharacter> CurrentNPC;

	bool bIsInConversation = false;
};
