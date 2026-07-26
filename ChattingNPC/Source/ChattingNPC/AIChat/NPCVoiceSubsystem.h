// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "NPCVoiceSubsystem.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * Turns an NPC's finished reply text into speech via the python_server /tts endpoint
 * and plays the returned WAV. Voice only; never mutates game state.
 *
 * Failure is non-fatal by design: the reply text is already on screen, so a TTS error
 * (server down, model not loaded, parse failure) is logged and silently dropped.
 * Only the most recent request is honored — a superseded/late response is discarded.
 */
UCLASS()
class CHATTINGNPC_API UNPCVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Synthesize and play speech for Text. No-op if TTS is disabled or the URL is empty.
	 * Pitch (semitones) and Speed differentiate per-NPC voices (single-speaker TTS).
	 * SpeakerActor (optional) positions the audio in 3D; null plays it as a 2D sound.
	 */
	UFUNCTION(BlueprintCallable, Category = "TTS")
	void SpeakForNPC(FName NPCId, const FString& Text, float Pitch = 0.0f, float Speed = 1.0f, AActor* SpeakerActor = nullptr);

	/** Stop current playback and discard any in-flight request (call on end / NPC switch). */
	UFUNCTION(BlueprintCallable, Category = "TTS")
	void StopSpeaking();

private:
	void HandleTtsResponse(FHttpResponsePtr Response, bool bConnectedSuccessfully,
		FName NPCId, uint64 RequestId, TWeakObjectPtr<AActor> SpeakerActor);

	/** Parse a 16-bit PCM WAV blob and play it (3D at SpeakerActor, or 2D if null). */
	void PlayWav(const TArray<uint8>& WavBytes, AActor* SpeakerActor);

	uint64 NextRequestId = 1;
	/** The only request whose response should play; others are stale. */
	uint64 ActiveRequestId = 0;

	/** Kept alive during playback so the wave/component are not GC'd mid-speech. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentAudioComponent;

	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> CurrentWave;
};
