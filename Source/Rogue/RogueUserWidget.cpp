// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueUserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void URogueUserWidget::UpdateHP(float Current, float Max)
{
    if (HPBar)
        HPBar->SetPercent(Current / Max);
}

void URogueUserWidget::UpdateStamina(float Current, float Max)
{
    if (StaminaBar)
        StaminaBar->SetPercent(Current / Max);
}

void URogueUserWidget::UpdateLevel(int32 Level)
{
    if (LevelText)
        LevelText->SetText(FText::FromString(FString::Printf(TEXT("Level : %d"), Level)));
}

void URogueUserWidget::UpdateExpBar(int32 CurrentExp, int32 NextExp)
{
    if (ExpBar)
    {
        float Ratio = (NextExp > 0) ? (float)CurrentExp / (float)NextExp : 0.0f;
        ExpBar->SetPercent(Ratio);
    }
}

void URogueUserWidget::UpdateSkillCooldown(float RemainingTime, float MaxCooldown)
{
    if (SkillCooldownText)
    {
        if (RemainingTime <= 0)
        {
            SkillCooldownText->SetText(FText::FromString(TEXT(" Skill\nReady")));
        }
        else
        {
            SkillCooldownText->SetText(FText::FromString(FString::Printf(TEXT(" Cool\n %.1f"), RemainingTime)));
        }
    }

    if (SkillCooldownBar && MaxCooldown > 0)
    {
        float Percent = FMath::Clamp(1.0f - (RemainingTime / MaxCooldown), 0.0f, 1.0f);
        SkillCooldownBar->SetPercent(Percent);
    }
}