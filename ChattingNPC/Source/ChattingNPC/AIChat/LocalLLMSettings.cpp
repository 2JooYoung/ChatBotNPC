// Copyright ChattingNPC. All Rights Reserved.

#include "AIChat/LocalLLMSettings.h"

ULocalLLMSettings::ULocalLLMSettings()
{
	ServerUrl = TEXT("http://127.0.0.1:8080/v1/chat/completions");
	ModelName = TEXT("local-model");
	DefaultTemperature = 0.7f;
	// Thinking-style models spend 200-300 tokens reasoning before the final
	// answer; 512 leaves room for both (llama.cpp strips the reasoning part).
	DefaultMaxTokens = 512;
	MaxHistoryMessages = 10;
	// Local CPU inference can be slow; 120s is a safer default than 30s.
	RequestTimeoutSeconds = 120.0f;

	// Voice (TTS) is opt-in; requires python_server running with MeloTTS loaded.
	bEnableTts = false;
	TtsServerUrl = TEXT("http://127.0.0.1:8000/tts");
	TtsRequestTimeoutSeconds = 60.0f;
}
