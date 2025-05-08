// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueCharacterStatComponent.h"
#include "RogueGameInstance.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
URogueCharacterStatComponent::URogueCharacterStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	Level = 1;
}


// Called when the game starts
void URogueCharacterStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void URogueCharacterStatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SetNewLevel(Level);
}

void URogueCharacterStatComponent::SetNewLevel(int32 NewLevel)
{
	URogueGameInstance* RogueGameInstance = Cast<URogueGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	if (!ensureMsgf(RogueGameInstance != nullptr, TEXT("RogueGameInstance is nullptr")))
	{
		return;
	}
	CurrentStatData = RogueGameInstance->GetRogueCharacterData(NewLevel);
	if (CurrentStatData != nullptr)
	{
		Level = NewLevel;
		CurrentHP = CurrentStatData->MaxHP;
		CurrentStamina = CurrentStatData->Stamina;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Level %d data doesn't exist"), NewLevel);
	}
}

void URogueCharacterStatComponent::SetDamage(float NewDamage)
{
	if (!ensureMsgf(CurrentStatData != nullptr, TEXT("CurrentStatData is nullptr")))
	{
		return;
	}

	CurrentHP = FMath::Clamp<float>(CurrentHP - NewDamage, 0, CurrentStatData->MaxHP);

	if (CurrentHP <= 0)
	{
		OnHPIsZero.Broadcast();
	}
}

float URogueCharacterStatComponent::GetAttack()
{
	if (CurrentStatData == nullptr)
	{
		return 0;
	}
	return CurrentStatData->Attack;
}

bool URogueCharacterStatComponent::ConsumeStamina(float Amount)
{
	if (CurrentStamina < Amount || !CurrentStatData) return false;

	CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.0f, CurrentStatData->Stamina);
	return true;
}

void URogueCharacterStatComponent::RecoverStamina(float DeltaTime)
{
	if (!CurrentStatData) return;

	CurrentStamina = FMath::Clamp(CurrentStamina + StaminaRecoveryRate * DeltaTime, 0.0f, CurrentStatData->Stamina);
}

void URogueCharacterStatComponent::AddExp(int32 ExpAmount)
{
	if (!CurrentStatData) return;

	CurrentExp += ExpAmount;

	while (CurrentStatData->NextExp > 0 && CurrentExp >= CurrentStatData->NextExp)
	{
		CurrentExp -= CurrentStatData->NextExp;
		SetNewLevel(Level + 1);

		URogueGameInstance* RogueGameInstance = Cast<URogueGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
		if (!RogueGameInstance) break;

		CurrentStatData = RogueGameInstance->GetRogueCharacterData(Level);
	}

	OnExpChanged.Broadcast(CurrentExp, GetNextExp());
}