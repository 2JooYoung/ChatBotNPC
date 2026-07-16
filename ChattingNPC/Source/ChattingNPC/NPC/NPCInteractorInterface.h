// Copyright ChattingNPC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPCInteractorInterface.generated.h"

class ANPCCharacter;

UINTERFACE(MinimalAPI)
class UNPCInteractorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by actors that can talk to NPCs (the player character in Phase 4).
 * An NPC calls these when its interaction range begins/stops overlapping the actor,
 * keeping the NPC decoupled from any concrete player class.
 */
class INPCInteractorInterface
{
	GENERATED_BODY()

public:
	virtual void NotifyNPCEnteredRange(ANPCCharacter* NPC) = 0;
	virtual void NotifyNPCExitedRange(ANPCCharacter* NPC) = 0;
};
