// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueGameInstance.h"

URogueGameInstance::URogueGameInstance()
{
	FString CharacterDataPath = TEXT("/Game/Data/RogueCharacterData.RogueCharacterData");

	static ConstructorHelpers::FObjectFinder<UDataTable> DT_CHARACTER(*CharacterDataPath);
	if (!ensureMsgf(DT_CHARACTER.Succeeded(), TEXT("DT_CHARACTER.Succeeded false")))
	{
		return;
	}
	RogueCharacterTable = DT_CHARACTER.Object;
	if (!ensureMsgf(RogueCharacterTable->GetRowMap().Num() > 0, TEXT("ABCharacterTable->GetRowMap().Num() > 0 false")))
	{
		return;
	}
}

void URogueGameInstance::Init()
{
	Super::Init();
}

FRogueCharacterData* URogueGameInstance::GetRogueCharacterData(int32 Level)
{
	return RogueCharacterTable->FindRow<FRogueCharacterData>(*FString::FromInt(Level), TEXT(""));
}