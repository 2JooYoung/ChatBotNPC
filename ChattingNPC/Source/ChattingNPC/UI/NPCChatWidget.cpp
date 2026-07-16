// Copyright ChattingNPC. All Rights Reserved.

#include "UI/NPCChatWidget.h"
#include "NPC/NPCCharacter.h"
#include "NPC/NPCProfileDataAsset.h"
#include "AIChat/LocalLLMSubsystem.h"
#include "ChattingNPC.h"

#include "Engine/GameInstance.h"

ULocalLLMSubsystem* UNPCChatWidget::GetLLM() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<ULocalLLMSubsystem>();
	}
	return nullptr;
}

void UNPCChatWidget::SubscribeToLLM()
{
	if (bSubscribed)
	{
		return;
	}
	if (ULocalLLMSubsystem* LLM = GetLLM())
	{
		LLM->OnResponseReceived.AddDynamic(this, &UNPCChatWidget::HandleLLMResponse);
		LLM->OnRequestFailed.AddDynamic(this, &UNPCChatWidget::HandleLLMFailure);
		bSubscribed = true;
	}
}

void UNPCChatWidget::UnsubscribeFromLLM()
{
	if (!bSubscribed)
	{
		return;
	}
	if (ULocalLLMSubsystem* LLM = GetLLM())
	{
		LLM->OnResponseReceived.RemoveDynamic(this, &UNPCChatWidget::HandleLLMResponse);
		LLM->OnRequestFailed.RemoveDynamic(this, &UNPCChatWidget::HandleLLMFailure);
	}
	bSubscribed = false;
}

void UNPCChatWidget::SetCurrentNPC(ANPCCharacter* NPC)
{
	CurrentNPC = NPC;
	CurrentProfile = NPC ? NPC->GetProfile() : nullptr;
	CurrentNPCId = NPC ? NPC->GetNPCId() : NAME_None;
}

void UNPCChatWidget::StartConversation(ANPCCharacter* NPC)
{
	if (!NPC)
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("ChatWidget: StartConversation called with null NPC."));
		return;
	}

	SetCurrentNPC(NPC);
	SubscribeToLLM();
	SetWaitingForResponse(false);

	OnConversationStarted(NPC->GetDisplayName(), NPC->GetRole());

	const FText Greeting = NPC->GetInitialGreeting();
	if (!Greeting.IsEmptyOrWhitespace())
	{
		AddNPCMessage(Greeting);
	}

	UE_LOG(LogChattingNPC, Log, TEXT("ChatWidget: conversation UI started for '%s'."), *CurrentNPCId.ToString());
}

void UNPCChatWidget::SendPlayerMessage(const FText& Message)
{
	if (bWaitingForResponse)
	{
		// Guard against double-send while a request is in flight.
		ShowErrorMessage(FText::FromString(TEXT("답변을 기다리는 중입니다.")));
		return;
	}

	const FString Trimmed = Message.ToString().TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		ShowErrorMessage(FText::FromString(TEXT("빈 메시지는 전송할 수 없습니다.")));
		return;
	}

	UNPCProfileDataAsset* Profile = CurrentProfile.Get();
	ULocalLLMSubsystem* LLM = GetLLM();
	if (!Profile || !LLM)
	{
		ShowErrorMessage(FText::FromString(TEXT("NPC 프로필이 설정되지 않았습니다.")));
		return;
	}

	AddPlayerMessage(FText::FromString(Trimmed));

	SetWaitingForResponse(true);
	OnRequestStarted();

	const bool bSent = LLM->SendMessageToNPC(Profile, Trimmed);
	if (!bSent)
	{
		// Immediate rejection already broadcast OnRequestFailed -> HandleLLMFailure
		// resets the waiting state; nothing else to do here.
		UE_LOG(LogChattingNPC, Verbose, TEXT("ChatWidget: send rejected immediately for '%s'."), *CurrentNPCId.ToString());
	}
}

void UNPCChatWidget::HandleLLMResponse(FName NPCId, const FString& Response)
{
	if (NPCId != CurrentNPCId)
	{
		return; // Response for a different NPC; ignore.
	}

	AddNPCMessage(FText::FromString(Response));
	SetWaitingForResponse(false);
	OnRequestCompleted();
}

void UNPCChatWidget::HandleLLMFailure(FName NPCId, const FString& UserMessage, int32 StatusCode)
{
	// NAME_None failures (e.g. null profile) are not tied to a specific NPC.
	if (!NPCId.IsNone() && NPCId != CurrentNPCId)
	{
		return;
	}

	SetWaitingForResponse(false);
	const FText ErrorText = FText::FromString(UserMessage);
	ShowErrorMessage(ErrorText);
	OnRequestFailed(ErrorText, StatusCode);
}

void UNPCChatWidget::AddPlayerMessage(const FText& Message)
{
	OnPlayerMessageAdded(Message);
}

void UNPCChatWidget::AddNPCMessage(const FText& Message)
{
	OnNPCMessageAdded(Message);
}

void UNPCChatWidget::ShowErrorMessage(const FText& ErrorMessage)
{
	UE_LOG(LogChattingNPC, Warning, TEXT("ChatWidget error: %s"), *ErrorMessage.ToString());
	OnErrorMessage(ErrorMessage);
}

void UNPCChatWidget::SetWaitingForResponse(bool bWaiting)
{
	bWaitingForResponse = bWaiting;
	OnWaitingForResponseChanged(bWaiting);
}

void UNPCChatWidget::EndConversation()
{
	// Close button -> ask the owner to tear everything down.
	OnCloseRequested.Broadcast();
}

void UNPCChatWidget::HandleConversationShutdown()
{
	UnsubscribeFromLLM();

	// Discard any in-flight response so a late reply is ignored.
	if (ULocalLLMSubsystem* LLM = GetLLM())
	{
		LLM->CancelRequest(CurrentNPCId);
	}

	SetWaitingForResponse(false);
	OnConversationEnded();

	UE_LOG(LogChattingNPC, Log, TEXT("ChatWidget: conversation UI shut down for '%s'."), *CurrentNPCId.ToString());
}

void UNPCChatWidget::NativeDestruct()
{
	// Safety net if the widget is destroyed without an explicit shutdown.
	UnsubscribeFromLLM();
	Super::NativeDestruct();
}
