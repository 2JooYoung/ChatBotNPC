// Copyright ChattingNPC. All Rights Reserved.

#include "ChattingNPC.h"
#include "Engine/Engine.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ChattingNPC, "ChattingNPC");

DEFINE_LOG_CATEGORY(LogChattingNPC);

void ChattingNPCScreenLog(const FString& Message, const FColor& Color, float DisplayTime)
{
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DisplayTime, Color, Message);
	}
#endif
}
