// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RogueUserWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 
 */
UCLASS()
class ROGUE_API URogueUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void UpdateHP(float Current, float Max);
    void UpdateStamina(float Current, float Max);
    void UpdateLevel(int32 Level);
    void UpdateExpBar(int32 CurrentExp, int32 NextExp);
    void UpdateSkillCooldown(float RemainingTime, float MaxCooldown);

protected:
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HPBar;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* StaminaBar;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* LevelText;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ExpBar;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* SkillCooldownText;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* SkillCooldownBar;
};
