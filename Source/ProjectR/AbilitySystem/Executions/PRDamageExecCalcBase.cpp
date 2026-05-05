// Copyright ProjectR. All Rights Reserved.

#include "PRDamageExecCalcBase.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "ProjectR/Combat/PRCombatTypes.h"

namespace
{
	// GE Context의 Instigator로부터 PlayerController를 해석. PlayerController/Pawn 양쪽 케이스 모두 처리
	APlayerController* ResolveInstigatorPlayerController(const FGameplayEffectContextHandle& ContextHandle)
	{
		AActor* InstigatorActor = ContextHandle.GetInstigator();
		if (!IsValid(InstigatorActor))
		{
			return nullptr;
		}

		if (APlayerController* DirectPC = Cast<APlayerController>(InstigatorActor))
		{
			return DirectPC;
		}
		
		if (APlayerState* InstigatorPS = Cast<APlayerState>(InstigatorActor))
		{
			return InstigatorPS->GetPlayerController();
		}

		if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			return Cast<APlayerController>(InstigatorPawn->GetController());
		}

		return nullptr;
	}
}

/*~ 후처리 디스패치 ~*/

void UPRDamageExecCalcBase::DispatchPostDamageApplied(
	AActor* TargetActor,
	const FGameplayEffectSpec& OwningSpec,
	const FPRDamageOutputs& Outputs,
	const FHitResult& HitResult,
	float CurrentHealth,
	float MaxHealth)
{
	IPRCombatInterface* CombatTarget = Cast<IPRCombatInterface>(TargetActor);
	if (CombatTarget == nullptr)
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = OwningSpec.GetContext();

	FPRDamageAppliedContext Context;
	Context.FinalDamage = Outputs.FinalDamage;
	Context.FinalGroggyDamage = Outputs.GroggyDamage;
	Context.HealthBeforeDamage = CurrentHealth;
	Context.HealthAfterDamage = FMath::Clamp(CurrentHealth - Outputs.FinalDamage, 0.0f, MaxHealth);
	Context.MaxHealth = MaxHealth;
	Context.Region = Outputs.Region;
	Context.Instigator = ContextHandle.GetOriginalInstigator();
	Context.InstigatorController = ResolveInstigatorPlayerController(ContextHandle);
	Context.HitResult = HitResult;
	Context.bIsCritical = Outputs.bIsCritical;
	CombatTarget->OnPostDamageApplied(Context);
}
