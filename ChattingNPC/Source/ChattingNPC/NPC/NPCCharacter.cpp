// Copyright ChattingNPC. All Rights Reserved.

#include "NPC/NPCCharacter.h"
#include "NPC/NPCProfileDataAsset.h"
#include "NPC/NPCInteractorInterface.h"
#include "ChattingNPC.h"

#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"

ANPCCharacter::ANPCCharacter()
{
	// Event-driven; no per-frame work needed.
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetCapsuleComponent());
	InteractionSphere->InitSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractionSphere->SetGenerateOverlapEvents(true);
}

void ANPCCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the sphere in sync with an edited radius in the editor.
	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractionRadius);
	}
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractionRadius);
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ANPCCharacter::OnInteractionSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ANPCCharacter::OnInteractionSphereEndOverlap);
	}

	if (!HasValidProfile())
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("NPC '%s' has no valid profile assigned."), *GetName());
	}
}

void ANPCCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (INPCInteractorInterface* Interactor = Cast<INPCInteractorInterface>(OtherActor))
	{
		Interactor->NotifyNPCEnteredRange(this);
	}

	OnPlayerEnteredRange(Pawn);
}

void ANPCCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (INPCInteractorInterface* Interactor = Cast<INPCInteractorInterface>(OtherActor))
	{
		Interactor->NotifyNPCExitedRange(this);
	}

	OnPlayerExitedRange(Pawn);
}

bool ANPCCharacter::HasValidProfile() const
{
	return Profile != nullptr && !Profile->NPCId.IsNone();
}

FName ANPCCharacter::GetNPCId() const
{
	return Profile ? Profile->NPCId : NAME_None;
}

FText ANPCCharacter::GetDisplayName() const
{
	return Profile ? Profile->NPCName : FText::GetEmpty();
}

FText ANPCCharacter::GetRole() const
{
	return Profile ? Profile->Role : FText::GetEmpty();
}

FText ANPCCharacter::GetInitialGreeting() const
{
	return Profile ? Profile->InitialGreeting : FText::GetEmpty();
}

void ANPCCharacter::StartConversation()
{
	if (bIsConversing)
	{
		return;
	}
	bIsConversing = true;
	OnConversationStarted();
}

void ANPCCharacter::EndConversation()
{
	if (!bIsConversing)
	{
		return;
	}
	bIsConversing = false;
	OnConversationEnded();
}
