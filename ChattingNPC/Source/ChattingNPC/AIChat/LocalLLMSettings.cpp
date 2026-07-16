// Copyright ChattingNPC. All Rights Reserved.

#include "AIChat/LocalLLMSettings.h"

ULocalLLMSettings::ULocalLLMSettings()
{
	ServerUrl = TEXT("http://127.0.0.1:8080/v1/chat/completions");
	ModelName = TEXT("local-model");
	DefaultTemperature = 0.7f;
	DefaultMaxTokens = 200;
	MaxHistoryMessages = 10;
	RequestTimeoutSeconds = 30.0f;
}
