// Copyright ChattingNPC. All Rights Reserved.

#include "Player/ChattingNPCPlayerCharacter.h"
#include "NPC/NPCCharacter.h"
#include "UI/NPCChatWidget.h"
#include "ChattingNPC.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

AChattingNPCPlayerCharacter::AChattingNPCPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Don't rotate the character with the controller; the camera boom handles that.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Rotate the character toward movement direction (third-person default).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
		Movement->JumpZVelocity = 700.0f;
		Movement->AirControl = 0.35f;
		Movement->MaxWalkSpeed = 500.0f;
		Movement->MinAnalogWalkSpeed = 20.0f;
		Movement->BrakingDecelerationWalking = 2000.0f;
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AChattingNPCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Register the default input mapping context (movement/look/jump/interact).
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			else
			{
				UE_LOG(LogChattingNPC, Warning,
					TEXT("Player: DefaultMappingContext not set. Assign IMC_Default in BP_Player defaults."));
				ChattingNPCScreenLog(TEXT("[Player] IMC_Default 미지정 (BP_Player 기본값에서 지정 필요)"), FColor::Red, 8.0f);
			}
		}
	}
}

void AChattingNPCPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}

void AChattingNPCPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AChattingNPCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AChattingNPCPlayerCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChattingNPCPlayerCharacter::Look);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AChattingNPCPlayerCharacter::OnInteract);
			UE_LOG(LogChattingNPC, Log, TEXT("Player: IA_Interact bound."));
		}
		else
		{
			UE_LOG(LogChattingNPC, Warning,
				TEXT("Player: InteractAction is not set. Assign IA_Interact in BP_Player defaults."));
			ChattingNPCScreenLog(TEXT("[Player] IA_Interact 미지정 (BP_Player 기본값에서 지정 필요)"), FColor::Red, 8.0f);
		}
	}
	else
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("Player: EnhancedInputComponent not found."));
	}
}

void AChattingNPCPlayerCharacter::NotifyNPCEnteredRange(ANPCCharacter* NPC)
{
	if (!NPC)
	{
		return;
	}

	NPCsInRange.AddUnique(NPC);
	UE_LOG(LogChattingNPC, Log, TEXT("Player: NPC '%s' entered range (%d in range)."),
		*NPC->GetNPCId().ToString(), NPCsInRange.Num());
	ChattingNPCScreenLog(FString::Printf(TEXT("[Player] E 키로 대화: %s"),
		*NPC->GetDisplayName().ToString()), FColor::Green);
}

void AChattingNPCPlayerCharacter::NotifyNPCExitedRange(ANPCCharacter* NPC)
{
	NPCsInRange.Remove(NPC);
	UE_LOG(LogChattingNPC, Log, TEXT("Player: NPC '%s' exited range (%d in range)."),
		NPC ? *NPC->GetNPCId().ToString() : TEXT("null"), NPCsInRange.Num());

	// If the player walked away from the NPC they were talking to, end it.
	if (bIsInConversation && CurrentNPC.Get() == NPC)
	{
		ChattingNPCScreenLog(TEXT("[Player] NPC 범위를 벗어나 대화를 종료합니다."), FColor::Orange);
		EndConversation();
	}
}

void AChattingNPCPlayerCharacter::OnInteract()
{
	UE_LOG(LogChattingNPC, Verbose, TEXT("Player: Interact pressed (InConversation=%d)."), bIsInConversation);

	if (bIsInConversation)
	{
		EndConversation();
		return;
	}

	ANPCCharacter* Nearest = FindNearestNPC();
	if (!Nearest)
	{
		ChattingNPCScreenLog(TEXT("[Player] 근처에 대화 가능한 NPC가 없습니다."), FColor::Yellow);
		return;
	}

	StartConversationWith(Nearest);
}

ANPCCharacter* AChattingNPCPlayerCharacter::FindNearestNPC() const
{
	ANPCCharacter* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector MyLocation = GetActorLocation();

	for (const TWeakObjectPtr<ANPCCharacter>& Weak : NPCsInRange)
	{
		ANPCCharacter* NPC = Weak.Get();
		if (!NPC)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLocation, NPC->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = NPC;
		}
	}

	return Best;
}

void AChattingNPCPlayerCharacter::StartConversationWith(ANPCCharacter* NPC)
{
	if (!NPC)
	{
		return;
	}

	if (!NPC->HasValidProfile())
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("Player: cannot start conversation, NPC '%s' has no valid profile."),
			*NPC->GetName());
		ChattingNPCScreenLog(TEXT("[Player] NPC 프로필이 설정되지 않았습니다."), FColor::Red);
		return;
	}

	CurrentNPC = NPC;
	bIsInConversation = true;
	NPC->StartConversation();
	SetConversationInputMode(true);

	// Create and show the chat widget (if a class is assigned).
	if (ChatWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		ChatWidget = CreateWidget<UNPCChatWidget>(PC ? PC : GetWorld()->GetFirstPlayerController(), ChatWidgetClass);
		if (ChatWidget)
		{
			ChatWidget->OnCloseRequested.AddDynamic(this, &AChattingNPCPlayerCharacter::HandleChatCloseRequested);
			ChatWidget->AddToViewport();
			ChatWidget->StartConversation(NPC);
		}
		else
		{
			UE_LOG(LogChattingNPC, Warning, TEXT("Player: failed to create chat widget."));
		}
	}
	else
	{
		UE_LOG(LogChattingNPC, Warning,
			TEXT("Player: ChatWidgetClass not set. Assign WBP_NPCChat in BP_ThirdPersonCharacter defaults."));
		ChattingNPCScreenLog(TEXT("[Player] ChatWidgetClass 미지정 (WBP_NPCChat 지정 필요)"), FColor::Red, 8.0f);
	}

	UE_LOG(LogChattingNPC, Log, TEXT("Player: conversation started with '%s'."), *NPC->GetNPCId().ToString());
	ChattingNPCScreenLog(FString::Printf(TEXT("[Player] 대화 시작: %s"),
		*NPC->GetDisplayName().ToString()), FColor::Green, 6.0f);

	OnConversationStartedWithNPC(NPC);
}

void AChattingNPCPlayerCharacter::HandleChatCloseRequested()
{
	EndConversation();
}

void AChattingNPCPlayerCharacter::EndConversation()
{
	if (!bIsInConversation)
	{
		return;
	}

	bIsInConversation = false;

	// Tear down the chat widget.
	if (ChatWidget)
	{
		ChatWidget->OnCloseRequested.RemoveDynamic(this, &AChattingNPCPlayerCharacter::HandleChatCloseRequested);
		ChatWidget->HandleConversationShutdown();
		ChatWidget->RemoveFromParent();
		ChatWidget = nullptr;
	}

	if (ANPCCharacter* NPC = CurrentNPC.Get())
	{
		NPC->EndConversation();
	}

	SetConversationInputMode(false);
	CurrentNPC = nullptr;

	UE_LOG(LogChattingNPC, Log, TEXT("Player: conversation ended, input restored."));
	ChattingNPCScreenLog(TEXT("[Player] 대화 종료 (이동/입력 복구)"), FColor::Orange, 6.0f);

	OnConversationEndedWithNPC();
}

void AChattingNPCPlayerCharacter::SetConversationInputMode(bool bEnable)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Stack-based; balanced by the single start/end pair guarded by bIsInConversation.
	PC->SetIgnoreMoveInput(bEnable);
	PC->SetIgnoreLookInput(bEnable);
	PC->SetShowMouseCursor(bEnable);

	if (bEnable)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}
