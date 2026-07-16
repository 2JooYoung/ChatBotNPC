// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LocalLLMSettings.generated.h"

/**
 * Project settings for the local LLM connection.
 * Server URL and model name are configurable here (never hardcoded in logic).
 * Editable under Project Settings > Plugins > Local LLM.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Local LLM"))
class CHATTINGNPC_API ULocalLLMSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULocalLLMSettings();

	/** OpenAI-compatible chat completions endpoint. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM")
	FString ServerUrl;

	/** Model name sent in the request body. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM")
	FString ModelName;

	/** Fallback temperature when a profile does not specify one. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DefaultTemperature;

	/** Fallback max tokens when a profile does not specify one. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM", meta = (ClampMin = "1"))
	int32 DefaultMaxTokens;

	/** Maximum number of stored user/assistant messages replayed as history. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM", meta = (ClampMin = "0"))
	int32 MaxHistoryMessages;

	/** HTTP request timeout in seconds. */
	UPROPERTY(Config, EditAnywhere, Category = "Local LLM", meta = (ClampMin = "1.0"))
	float RequestTimeoutSeconds;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
