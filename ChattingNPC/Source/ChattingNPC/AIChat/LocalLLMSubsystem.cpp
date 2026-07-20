// Copyright ChattingNPC. All Rights Reserved.

#include "AIChat/LocalLLMSubsystem.h"
#include "AIChat/LocalLLMSettings.h"
#include "AIChat/NPCSystemPromptBuilder.h"
#include "NPC/NPCProfileDataAsset.h"
#include "ChattingNPC.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"

// User-facing error messages (Korean). Kept centralized for reuse.
namespace LLMUserMessages
{
	static const TCHAR* NoProfile = TEXT("NPC 프로필이 설정되지 않았습니다.");
	static const TCHAR* EmptyMessage = TEXT("빈 메시지는 전송할 수 없습니다.");
	static const TCHAR* Busy = TEXT("답변을 기다리는 중입니다.");
	static const TCHAR* CannotConnect = TEXT("대화 서버에 연결할 수 없습니다.");
	static const TCHAR* GenerationFailed = TEXT("NPC의 답변을 생성하지 못했습니다.");
}

void ULocalLLMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Log responses/failures during development for Phase 2 verification.
	OnResponseReceived.AddDynamic(this, &ULocalLLMSubsystem::HandleDebugResponse);
	OnRequestFailed.AddDynamic(this, &ULocalLLMSubsystem::HandleDebugFailure);

	TestConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("llm.test"),
		TEXT("Send a fixed test message to the local LLM and log the response."),
		FConsoleCommandDelegate::CreateUObject(this, &ULocalLLMSubsystem::SendTestMessage),
		ECVF_Default);

	HistoryConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("llm.history"),
		TEXT("Log every NPC's stored conversation history (session independence / trim check)."),
		FConsoleCommandDelegate::CreateUObject(this, &ULocalLLMSubsystem::DumpConversationHistory),
		ECVF_Default);

	UE_LOG(LogChattingNPC, Log, TEXT("LocalLLMSubsystem initialized. Type 'llm.test' in the console to verify connectivity."));
}

void ULocalLLMSubsystem::Deinitialize()
{
	if (TestConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(TestConsoleCommand);
		TestConsoleCommand = nullptr;
	}

	if (HistoryConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(HistoryConsoleCommand);
		HistoryConsoleCommand = nullptr;
	}

	// In-flight HTTP callbacks capture a weak pointer to this, so pending
	// responses after teardown are safely ignored.
	Super::Deinitialize();
}

FNPCConversationSession& ULocalLLMSubsystem::GetOrCreateSession(FName NPCId)
{
	if (FNPCConversationSession* Existing = Sessions.Find(NPCId))
	{
		return *Existing;
	}

	FNPCConversationSession NewSession;
	NewSession.NPCId = NPCId;
	return Sessions.Add(NPCId, MoveTemp(NewSession));
}

void ULocalLLMSubsystem::ClearConversation(FName NPCId)
{
	Sessions.Remove(NPCId);
	ActiveRequestIds.Remove(NPCId);
	InFlightRequests.Remove(NPCId);
}

void ULocalLLMSubsystem::ClearAllConversations()
{
	Sessions.Reset();
	ActiveRequestIds.Reset();
	InFlightRequests.Reset();
}

void ULocalLLMSubsystem::CancelRequest(FName NPCId)
{
	// Drop validity so a late response is discarded; allow a fresh request to start.
	ActiveRequestIds.Remove(NPCId);
	InFlightRequests.Remove(NPCId);
}

bool ULocalLLMSubsystem::SendMessageToNPC(UNPCProfileDataAsset* Profile, const FString& PlayerMessage)
{
	if (!Profile)
	{
		OnRequestFailed.Broadcast(NAME_None, LLMUserMessages::NoProfile, 0);
		return false;
	}

	const FString Trimmed = PlayerMessage.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		OnRequestFailed.Broadcast(Profile->NPCId, LLMUserMessages::EmptyMessage, 0);
		return false;
	}

	const FName NPCId = Profile->NPCId;
	if (InFlightRequests.Contains(NPCId))
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::Busy, 0);
		return false;
	}

	const ULocalLLMSettings* Settings = GetDefault<ULocalLLMSettings>();
	if (!Settings || Settings->ServerUrl.IsEmpty())
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::CannotConnect, 0);
		return false;
	}

	// Resolve per-NPC generation params, falling back to project defaults.
	const float Temperature = Profile->Temperature >= 0.0f ? Profile->Temperature : Settings->DefaultTemperature;
	const int32 MaxTokens = Profile->MaxResponseTokens > 0 ? Profile->MaxResponseTokens : Settings->DefaultMaxTokens;

	// Build the OpenAI-compatible request body.
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Settings->ModelName);
	Root->SetNumberField(TEXT("temperature"), Temperature);
	Root->SetNumberField(TEXT("max_tokens"), MaxTokens);
	Root->SetBoolField(TEXT("stream"), false);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	auto MakeMessage = [](const FString& Role, const FString& Content) -> TSharedPtr<FJsonValue>
	{
		const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("role"), Role);
		Obj->SetStringField(TEXT("content"), Content);
		return MakeShared<FJsonValueObject>(Obj);
	};

	MessagesArray.Add(MakeMessage(NPCChatRoles::System, FNPCSystemPromptBuilder::Build(Profile, DialogueContext)));

	const FNPCConversationSession& Session = GetOrCreateSession(NPCId);
	for (const FNPCChatMessage& Message : Session.Messages)
	{
		MessagesArray.Add(MakeMessage(Message.Role, Message.Content));
	}

	MessagesArray.Add(MakeMessage(NPCChatRoles::User, Trimmed));
	Root->SetArrayField(TEXT("messages"), MessagesArray);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	// Track this request for de-duplication and stale-response detection.
	const uint64 RequestId = NextRequestId++;
	ActiveRequestIds.Add(NPCId, RequestId);
	InFlightRequests.Add(NPCId);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Settings->ServerUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);
	Request->SetTimeout(Settings->RequestTimeoutSeconds);

	TWeakObjectPtr<ULocalLLMSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, NPCId, RequestId, Trimmed]
		(FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (ULocalLLMSubsystem* Self = WeakThis.Get())
			{
				Self->HandleResponse(Response, bConnectedSuccessfully, NPCId, RequestId, Trimmed);
			}
		});

	Request->ProcessRequest();

	UE_LOG(LogChattingNPC, Log, TEXT("Request sent to '%s' for NPC '%s' (model='%s', max_tokens=%d, timeout=%.0fs). Waiting..."),
		*Settings->ServerUrl, *NPCId.ToString(), *Settings->ModelName, MaxTokens, Settings->RequestTimeoutSeconds);
	return true;
}

void ULocalLLMSubsystem::HandleResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully,
	FName NPCId, uint64 RequestId, FString PlayerMessage)
{
	// This NPC no longer has this request in flight.
	InFlightRequests.Remove(NPCId);

	// If the conversation ended/cleared (or a newer request superseded this one),
	// the stored id won't match: discard the late response silently.
	const uint64* Current = ActiveRequestIds.Find(NPCId);
	if (!Current || *Current != RequestId)
	{
		UE_LOG(LogChattingNPC, Verbose, TEXT("Discarding stale/late response for NPC '%s'."), *NPCId.ToString());
		return;
	}
	ActiveRequestIds.Remove(NPCId);

	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::CannotConnect, 0);
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	if (StatusCode != 200)
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::GenerationFailed, StatusCode);
		return;
	}

	FString Content;
	if (!ParseResponseContent(Response->GetContentAsString(), Content))
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::GenerationFailed, StatusCode);
		return;
	}

	Content = Content.TrimStartAndEnd();
	if (Content.IsEmpty())
	{
		OnRequestFailed.Broadcast(NPCId, LLMUserMessages::GenerationFailed, StatusCode);
		return;
	}

	// Persist both turns only on success, then trim to the history window.
	FNPCConversationSession& Session = GetOrCreateSession(NPCId);
	Session.AddMessage(NPCChatRoles::User, PlayerMessage);
	Session.AddMessage(NPCChatRoles::Assistant, Content);

	const ULocalLLMSettings* Settings = GetDefault<ULocalLLMSettings>();
	const int32 MaxHistory = Settings ? Settings->MaxHistoryMessages : 10;
	const int32 NumBeforeTrim = Session.Messages.Num();
	Session.TrimHistory(MaxHistory);

	const int32 NumTrimmed = NumBeforeTrim - Session.Messages.Num();
	if (NumTrimmed > 0)
	{
		UE_LOG(LogChattingNPC, Log, TEXT("[%s] history full: dropped %d oldest message(s)."),
			*NPCId.ToString(), NumTrimmed);
	}
	UE_LOG(LogChattingNPC, Log, TEXT("[%s] history: %d/%d messages stored."),
		*NPCId.ToString(), Session.Messages.Num(), MaxHistory);
	ChattingNPCScreenLog(FString::Printf(TEXT("[대화기록] %s: %d/%d%s"),
		*NPCId.ToString(), Session.Messages.Num(), MaxHistory,
		NumTrimmed > 0 ? TEXT(" (오래된 기록 제거됨)") : TEXT("")), FColor::Emerald);

	OnResponseReceived.Broadcast(NPCId, Content);
}

bool ULocalLLMSubsystem::ParseResponseContent(const FString& Body, FString& OutContent)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
	if (!Root->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* FirstChoice = nullptr;
	if (!(*Choices)[0]->TryGetObject(FirstChoice) || !FirstChoice)
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* MessageObj = nullptr;
	if (!(*FirstChoice)->TryGetObjectField(TEXT("message"), MessageObj) || !MessageObj)
	{
		return false;
	}

	FString ContentValue;
	if (!(*MessageObj)->TryGetStringField(TEXT("content"), ContentValue))
	{
		return false;
	}

	OutContent = ContentValue;
	return true;
}

void ULocalLLMSubsystem::SendTestMessage()
{
	TestProfile = NewObject<UNPCProfileDataAsset>(this);
	TestProfile->NPCId = TEXT("test_npc");
	TestProfile->NPCName = FText::FromString(TEXT("테스트 NPC"));
	TestProfile->Role = FText::FromString(TEXT("마을 안내인"));
	TestProfile->Personality = TEXT("친절하고 침착함");
	TestProfile->SpeakingStyle = TEXT("정중하고 간결함");
	TestProfile->Background = TEXT("작은 마을의 안내를 맡고 있다.");
	TestProfile->KnownInformation = { TEXT("마을 지리"), TEXT("주민들") };
	TestProfile->Temperature = 0.7f;
	// Thinking-style models need reasoning headroom; must match the 512 default.
	TestProfile->MaxResponseTokens = 512;

	UE_LOG(LogChattingNPC, Log, TEXT("Sending test message to local LLM..."));
	SendMessageToNPC(TestProfile, TEXT("안녕하세요, 이 마을에 대해 간단히 알려줄 수 있나요?"));
}

void ULocalLLMSubsystem::DumpConversationHistory()
{
	UE_LOG(LogChattingNPC, Log, TEXT("=== Conversation history: %d session(s) ==="), Sessions.Num());
	ChattingNPCScreenLog(FString::Printf(TEXT("[대화기록 덤프] 세션 %d개 — 자세한 내용은 Output Log 참고"), Sessions.Num()), FColor::Yellow, 6.0f);

	for (const TPair<FName, FNPCConversationSession>& Pair : Sessions)
	{
		const FNPCConversationSession& Session = Pair.Value;
		UE_LOG(LogChattingNPC, Log, TEXT("--- [%s] %d message(s) (in-flight: %s) ---"),
			*Pair.Key.ToString(), Session.Messages.Num(),
			InFlightRequests.Contains(Pair.Key) ? TEXT("yes") : TEXT("no"));

		for (int32 Index = 0; Index < Session.Messages.Num(); ++Index)
		{
			const FNPCChatMessage& Message = Session.Messages[Index];
			UE_LOG(LogChattingNPC, Log, TEXT("  %2d. [%s] %s"),
				Index + 1, *Message.Role, *Message.Content.Left(120));
		}
	}
}

void ULocalLLMSubsystem::HandleDebugResponse(FName NPCId, const FString& Response)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogChattingNPC, Log, TEXT("[%s] response: %s"), *NPCId.ToString(), *Response);
#endif
}

void ULocalLLMSubsystem::HandleDebugFailure(FName NPCId, const FString& UserMessage, int32 StatusCode)
{
	UE_LOG(LogChattingNPC, Warning, TEXT("[%s] request failed (code %d): %s"),
		*NPCId.ToString(), StatusCode, *UserMessage);
}
