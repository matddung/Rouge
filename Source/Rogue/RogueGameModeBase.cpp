// Copyright Epic Games, Inc. All Rights Reserved.


#include "RogueGameModeBase.h"
#include "UObject/ConstructorHelpers.h"

ARogueGameModeBase::ARogueGameModeBase()
{
	static ConstructorHelpers::FClassFinder<APawn> Player(TEXT("/Game/Blueprints/BP_RogueCharacter"));
	if (Player.Succeeded())
	{
		DefaultPawnClass = Player.Class;
	}
}