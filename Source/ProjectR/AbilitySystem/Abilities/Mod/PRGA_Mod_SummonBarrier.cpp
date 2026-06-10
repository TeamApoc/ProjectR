// Copyright ProjectR. All Rights Reserved.

#include "PRGA_Mod_SummonBarrier.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Projectile/PRGroundBoxProjectileBase.h"

UPRGA_Mod_SummonBarrier::UPRGA_Mod_SummonBarrier()
{
	PlayerAbilityType = EPRPlayerAbilityType::Mod;
	InputTag = PRGameplayTags::Input_Ability_Mod;
}

/*~ UGameplayAbility Interface ~*/

bool UPRGA_Mod_SummonBarrier::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (HasLaunchActivationWindow())
	{
		// 발사 비용 우회
		return UGameplayAbility::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UPRGA_Mod_SummonBarrier::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (HasLaunchActivationWindow())
	{
		// 발사 추가 비용 없음
		return;
	}

	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Mod_SummonBarrier::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (HasLaunchActivationWindow())
	{
		// 발사 경로
		UPRGA_Mod::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

		if (!HasAuthority(&ActivationInfo))
		{
			bLaunchRequested = true;
			UnbindDurationCostRemovalEvent();
			ActiveDurationCostHandle.Invalidate();
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		const bool bLaunched = LaunchActiveBarrier(ActorInfo);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, !bLaunched);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPRGA_Mod_SummonBarrier::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	RequestActiveBarrierEnd();
	CleanupBarrierRuntime();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

/*~ UPRGA_Mod_HasDuration Interface ~*/

void UPRGA_Mod_SummonBarrier::OnDurationStarted_Implementation()
{
	Super::OnDurationStarted_Implementation();

	ActiveDurationCostHandle = GetLastAppliedModCostHandle();
	BindDurationCostRemovalEvent();

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!IsValid(BarrierActorClass) || !IsValid(SpawnBarrier(GetCurrentActorInfo())))
	{
		CleanupBarrierRuntime();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	BindSurvivalTagEvents();
}

/*~ 배리어 상태 ~*/

bool UPRGA_Mod_SummonBarrier::HasActiveBarrier() const
{
	return IsValid(ActiveBarrier) && !bLaunchRequested;
}

bool UPRGA_Mod_SummonBarrier::HasLaunchActivationWindow() const
{
	return !bLaunchRequested && (HasActiveBarrier() || ActiveDurationCostHandle.IsValid());
}

APRGroundBoxProjectileBase* UPRGA_Mod_SummonBarrier::SpawnBarrier(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ActorInfo == nullptr || !IsValid(BarrierActorClass))
	{
		return nullptr;
	}

	APawn* PlayerPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerPawn) || !PlayerPawn->HasAuthority())
	{
		return nullptr;
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	RequestActiveBarrierEnd();
	bLaunchRequested = false;

	const FVector SpawnLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorRotation().RotateVector(SpawnOffset);
	const FRotator SpawnRotation = PlayerPawn->GetActorRotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerPawn;
	SpawnParameters.Instigator = PlayerPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APRGroundBoxProjectileBase* SpawnedBarrier = World->SpawnActor<APRGroundBoxProjectileBase>(
		BarrierActorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
	if (!IsValid(SpawnedBarrier))
	{
		return nullptr;
	}

	FPRGroundBoxLaunchParams LaunchParams;
	LaunchParams.SourceActor = PlayerPawn;
	LaunchParams.DamageEffectSpec = MakeModEffectSpec(BarrierDamage, BarrierGroggyDamage);
	LaunchParams.OverrideMaxHealth = BarrierMaxHealth;
	SpawnedBarrier->InitGroundBoxProjectile(LaunchParams);
	SpawnedBarrier->OnGroundBoxDestroyed.AddDynamic(this, &ThisClass::HandleBarrierDestroyed);

	ActiveBarrier = SpawnedBarrier;
	return SpawnedBarrier;
}

bool UPRGA_Mod_SummonBarrier::LaunchActiveBarrier(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!HasActiveBarrier())
	{
		return false;
	}

	const FVector LaunchDirection = ResolveLaunchDirection(ActorInfo);
	if (LaunchDirection.IsNearlyZero() || LaunchSpeed <= 0.0f)
	{
		return false;
	}

	APRGroundBoxProjectileBase* BarrierToLaunch = ActiveBarrier;
	bLaunchRequested = true;
	UnbindDurationCostRemovalEvent();
	UnbindSurvivalTagEvents();

	BarrierToLaunch->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleBarrierDestroyed);
	BarrierToLaunch->LaunchGroundBoxProjectile(LaunchDirection, LaunchSpeed);

	ActiveBarrier = nullptr;
	return true;
}

void UPRGA_Mod_SummonBarrier::RequestActiveBarrierEnd()
{
	if (IsValid(ActiveBarrier) && ActiveBarrier->HasAuthority())
	{
		// 활성 배리어 제거
		ActiveBarrier->RequestGroundBoxEnd(EPRProjectileDestroyReason::Manual);
	}

	ActiveBarrier = nullptr;
	bLaunchRequested = false;
}

void UPRGA_Mod_SummonBarrier::CleanupBarrierRuntime()
{
	UnbindDurationCostRemovalEvent();
	UnbindSurvivalTagEvents();

	if (IsValid(ActiveBarrier))
	{
		ActiveBarrier->OnGroundBoxDestroyed.RemoveDynamic(this, &ThisClass::HandleBarrierDestroyed);
	}

	ActiveDurationCostHandle.Invalidate();
	ActiveBarrier = nullptr;
	bLaunchRequested = false;
}

/*~ 이벤트 바인딩 ~*/

void UPRGA_Mod_SummonBarrier::BindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || DurationCostRemovedDelegateHandle.IsValid())
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		// 비용 종료 감시
		DurationCostRemovedDelegateHandle = RemovedDelegate->AddUObject(this, &ThisClass::HandleDurationCostRemoved);
	}
}

void UPRGA_Mod_SummonBarrier::UnbindDurationCostRemovalEvent()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC) || !ActiveDurationCostHandle.IsValid() || !DurationCostRemovedDelegateHandle.IsValid())
	{
		DurationCostRemovedDelegateHandle.Reset();
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = ASC->OnGameplayEffectRemoved_InfoDelegate(ActiveDurationCostHandle);
	if (RemovedDelegate != nullptr)
	{
		RemovedDelegate->Remove(DurationCostRemovedDelegateHandle);
	}

	DurationCostRemovedDelegateHandle.Reset();
}

void UPRGA_Mod_SummonBarrier::BindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	if (!DeadTagEventHandle.IsValid())
	{
		// 사망 감시
		DeadTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::HandleSurvivalTagChanged);
	}

	if (!DownTagEventHandle.IsValid())
	{
		// 다운 감시
		DownTagEventHandle = ASC->RegisterGameplayTagEvent(PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::HandleSurvivalTagChanged);
	}
}

void UPRGA_Mod_SummonBarrier::UnbindSurvivalTagEvents()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		DeadTagEventHandle.Reset();
		DownTagEventHandle.Reset();
		return;
	}

	if (DeadTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DeadTagEventHandle, PRGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
		DeadTagEventHandle.Reset();
	}

	if (DownTagEventHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(DownTagEventHandle, PRGameplayTags::State_Down, EGameplayTagEventType::NewOrRemoved);
		DownTagEventHandle.Reset();
	}
}

/*~ 이벤트 처리 ~*/

FVector UPRGA_Mod_SummonBarrier::ResolveLaunchDirection(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo == nullptr)
	{
		return FVector::ZeroVector;
	}

	const APawn* PlayerPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerPawn))
	{
		return FVector::ZeroVector;
	}

	if (const AController* Controller = PlayerPawn->GetController())
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FVector ControlDirection = FRotator(0.0f, ControlRotation.Yaw, 0.0f).Vector();
		if (!ControlDirection.IsNearlyZero())
		{
			return ControlDirection.GetSafeNormal();
		}
	}

	FVector ActorDirection = PlayerPawn->GetActorForwardVector();
	ActorDirection.Z = 0.0f;
	return ActorDirection.GetSafeNormal();
}

void UPRGA_Mod_SummonBarrier::HandleDurationCostRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	// 비용 종료
	DurationCostRemovedDelegateHandle.Reset();
	ActiveDurationCostHandle.Invalidate();

	if (!bLaunchRequested)
	{
		RequestActiveBarrierEnd();
	}

	CleanupBarrierRuntime();
}

void UPRGA_Mod_SummonBarrier::HandleSurvivalTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	if (Tag == PRGameplayTags::State_Dead || Tag == PRGameplayTags::State_Down)
	{
		RequestActiveBarrierEnd();
		CleanupBarrierRuntime();
	}
}

void UPRGA_Mod_SummonBarrier::HandleBarrierDestroyed(APRGroundBoxProjectileBase* DestroyedBarrier)
{
	if (DestroyedBarrier != ActiveBarrier)
	{
		return;
	}

	// 배리어 참조 정리
	CleanupBarrierRuntime();
}
