#include "NormalEnemyAnimInstance.h"
#include "EnemyBase.h"

void UNormalEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	APawn* Pawn = TryGetPawnOwner();

	if (Pawn)
	{
		Speed = Pawn->GetVelocity().Size();
	}
}

void UNormalEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OnMontageEnded.AddDynamic(this, &UNormalEnemyAnimInstance::HandleMontageEnded);

	if (auto* OwnerPawn = Cast<AEnemyBase>(TryGetPawnOwner()))
	{
		if (OwnerPawn->AttackMontage)
		{
			AttackMontage = OwnerPawn->AttackMontage;
		}
	}
}

void UNormalEnemyAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
	OnMontageEnded.RemoveDynamic(this, &UNormalEnemyAnimInstance::HandleMontageEnded);
}

void UNormalEnemyAnimInstance::AnimNotify_AttackHitCheck()
{
	OnAttackHitCheck.Broadcast();
}

void UNormalEnemyAnimInstance::AnimNotify_AttackEnd()
{
	OnAttackEnd.Broadcast();
}

void UNormalEnemyAnimInstance::PlayRandomAttack()
{
	if (!AttackMontage || Montage_IsPlaying(AttackMontage)) return;

	if (Montage_IsPlaying(AttackMontage))
	{
		return;
	}

	int32 NumSections = AttackMontage->CompositeSections.Num();
	int32 Index = FMath::RandRange(0, NumSections - 1);
	FName Section = AttackMontage->CompositeSections[Index].SectionName;

	Montage_Play(AttackMontage, 1.f);
	Montage_JumpToSection(Section, AttackMontage);
}

void UNormalEnemyAnimInstance::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == AttackMontage)
	{
		OnAttackEnd.Broadcast();
	}
}