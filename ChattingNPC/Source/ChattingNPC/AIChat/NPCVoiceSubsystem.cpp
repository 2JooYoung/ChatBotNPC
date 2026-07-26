// Copyright ChattingNPC. All Rights Reserved.

#include "AIChat/NPCVoiceSubsystem.h"
#include "AIChat/LocalLLMSettings.h"
#include "ChattingNPC.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Audio.h"
#include "Sound/SoundWaveProcedural.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UNPCVoiceSubsystem::SpeakForNPC(FName NPCId, const FString& Text, float Pitch, float Speed, AActor* SpeakerActor)
{
	const ULocalLLMSettings* Settings = GetDefault<ULocalLLMSettings>();
	if (!Settings || !Settings->bEnableTts || Settings->TtsServerUrl.IsEmpty())
	{
		return; // Voice disabled or unconfigured — nothing to do.
	}

	const FString Trimmed = Text.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}

	// A new utterance supersedes any previous one: stop current audio and
	// invalidate any in-flight request before issuing this one.
	StopSpeaking();

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("text"), Trimmed);
	Root->SetNumberField(TEXT("pitch"), Pitch);
	Root->SetNumberField(TEXT("speed"), Speed);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);

	const uint64 RequestId = NextRequestId++;
	ActiveRequestId = RequestId;

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Settings->TtsServerUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body); // UTF-8 — matches the server's expectation.
	Request->SetTimeout(Settings->TtsRequestTimeoutSeconds);

	TWeakObjectPtr<UNPCVoiceSubsystem> WeakThis(this);
	TWeakObjectPtr<AActor> WeakSpeaker(SpeakerActor);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, NPCId, RequestId, WeakSpeaker]
		(FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (UNPCVoiceSubsystem* Self = WeakThis.Get())
			{
				Self->HandleTtsResponse(Response, bConnectedSuccessfully, NPCId, RequestId, WeakSpeaker);
			}
		});

	Request->ProcessRequest();
	UE_LOG(LogChattingNPC, Verbose, TEXT("TTS request sent for NPC '%s' (%d chars)."), *NPCId.ToString(), Trimmed.Len());
}

void UNPCVoiceSubsystem::StopSpeaking()
{
	// Any pending response no longer matches -> it will be discarded.
	ActiveRequestId = 0;

	if (CurrentAudioComponent)
	{
		CurrentAudioComponent->Stop();
		CurrentAudioComponent = nullptr;
	}
	CurrentWave = nullptr;
}

void UNPCVoiceSubsystem::HandleTtsResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully,
	FName NPCId, uint64 RequestId, TWeakObjectPtr<AActor> SpeakerActor)
{
	// Superseded by a newer utterance, or StopSpeaking() was called: ignore.
	if (RequestId != ActiveRequestId)
	{
		return;
	}

	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("TTS: no response for NPC '%s' (voice skipped)."), *NPCId.ToString());
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	if (StatusCode != 200)
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("TTS: server returned %d for NPC '%s' (voice skipped)."), StatusCode, *NPCId.ToString());
		return;
	}

	PlayWav(Response->GetContent(), SpeakerActor.Get());
}

void UNPCVoiceSubsystem::PlayWav(const TArray<uint8>& WavBytes, AActor* SpeakerActor)
{
	if (WavBytes.Num() == 0)
	{
		return;
	}

	FWaveModInfo WaveInfo;
	if (!WaveInfo.ReadWaveInfo(WavBytes.GetData(), WavBytes.Num()))
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("TTS: failed to parse WAV header (voice skipped)."));
		return;
	}

	const int32 BitsPerSample = *WaveInfo.pBitsPerSample;
	if (BitsPerSample != 16)
	{
		UE_LOG(LogChattingNPC, Warning, TEXT("TTS: expected 16-bit PCM, got %d-bit (voice skipped)."), BitsPerSample);
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const int32 NumChannels = *WaveInfo.pChannels;
	const int32 SampleRate = *WaveInfo.pSamplesPerSec;
	const int32 PCMDataSize = static_cast<int32>(WaveInfo.SampleDataSize);
	if (NumChannels <= 0 || SampleRate <= 0 || PCMDataSize <= 0)
	{
		return;
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = NumChannels;
	// Real duration (not INDEFINITELY_LOOPING) so the one-shot stops when drained.
	Wave->Duration = static_cast<float>(PCMDataSize) / (NumChannels * sizeof(int16) * SampleRate);
	// Queue the full clip up-front; the procedural wave drains it during playback.
	Wave->QueueAudio(WaveInfo.SampleDataStart, PCMDataSize);

	UAudioComponent* Comp = SpeakerActor
		? UGameplayStatics::SpawnSoundAtLocation(World, Wave, SpeakerActor->GetActorLocation())
		: UGameplayStatics::SpawnSound2D(World, Wave);

	// Retain so neither the wave nor the component is GC'd mid-playback.
	CurrentWave = Wave;
	CurrentAudioComponent = Comp;

	UE_LOG(LogChattingNPC, Verbose, TEXT("TTS: playing %.2fs (%d Hz, %d ch)."), Wave->Duration, SampleRate, NumChannels);
}
