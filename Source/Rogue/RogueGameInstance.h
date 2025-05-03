// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "RogueGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FRogueCharacterData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FRogueCharacterData() : Level(1), MaxHP(100.0f), Attack(10.0f), NextExp(30) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float MaxHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	float Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	int32 NextExp;
};

/**
 * 
 */
UCLASS()
class ROGUE_API URogueGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	URogueGameInstance();

	virtual void Init() override;
	FRogueCharacterData* GetRogueCharacterData(int32 Level);

private:
	UPROPERTY()
	class UDataTable* RogueCharacterTable;
};
