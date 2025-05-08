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

void URogueUserWidget::UpdateBarWidths(float MaxHP, float MaxStamina)
{
    if (SB_HP)
    {
        SB_HP->SetWidthOverride(MaxHP * 2.f);
    }

    if (SB_Stamina)
    {
        SB_Stamina->SetWidthOverride(MaxStamina * 2.f);
    }
}

void URogueUserWidget::UpdateExpBar(int32 CurrentExp, int32 NextExp)
{
    if (ExpBar)
    {
        float Ratio = (NextExp > 0) ? (float)CurrentExp / (float)NextExp : 0.0f;
        ExpBar->SetPercent(Ratio);
    }
}