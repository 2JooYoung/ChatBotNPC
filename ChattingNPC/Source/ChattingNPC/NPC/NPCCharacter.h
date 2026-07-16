// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCCharacter.generated.h"

class USphereComponent;
class UNPCProfileDataAsset;

/**
 * Base NPC the player can talk to. Owns an interaction-range sphere and a profile.
 * Detects player enter/leave (no Tick) and exposes conversation start/end.
 * Generates no game state itself; dialogue is handled by ULocalLLMSubsystem.
 */
UCLASS()
class CHATTINGNPC_API ANPCCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCCharacter();

	/** Profile driving this NPC's persona and generation params (may be null if unset). */
	UFUNCTION(BlueprintPure, Category = "NPC")
	UNPCProfileDataAsset* GetProfile() const { return Profile; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	FName GetNPCId() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetRole() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetInitialGreeting() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool HasValidProfile() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool IsConversing() const { return bIsConversing; }

	/** Marks this NPC as in-conversation and fires the Blueprint hook. */
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void StartConversation();

	/** Clears the in-conversation flag and fires the Blueprint hook. */
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void EndConversation();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//~ Blueprint hooks (e.g. show/hide "Press E to talk" prompt).
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void OnPlayerEnteredRange(APawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void OnPlayerExitedRange(APawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void OnConversationStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC")
	void OnConversationEnded();

	/** Sphere defining how close the player must be to interact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** Assign a DA_NPC_* asset per NPC instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UNPCProfileDataAsset> Profile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC", meta = (ClampMin = "0.0"))
	float InteractionRadius = 220.0f;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	bool bIsConversing = false;
};
