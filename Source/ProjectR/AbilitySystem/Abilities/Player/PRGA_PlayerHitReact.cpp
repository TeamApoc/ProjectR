// Copyright ProjectR. All Rights Reserved.

#include "PRGA_PlayerHitReact.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"

UPRGA_PlayerHitReact::UPRGA_PlayerHitReact()
{
	FGameplayTagContainer DefaultAbilityTags;
	DefaultAbilityTags.AddTag(PRGameplayTags::Ability_Player_HitReact);
	SetAssetTags(DefaultAbilityTags);
	ActivationBlockedTags.AddTag(PRGameplayTags::State_Dead);

	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Aim);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Crouch);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Sprint);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Reload);
	BlockAbilitiesWithTag.AddTag(PRGameplayTags::Ability_Player_Interaction);

	FAbilityTriggerData WeakTriggerData;
	WeakTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	WeakTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Weak;
	AbilityTriggers.Add(WeakTriggerData);

	FAbilityTriggerData StrongTriggerData;
	StrongTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	StrongTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Strong;
	AbilityTriggers.Add(StrongTriggerData);

	FAbilityTriggerData DownTriggerData;
	DownTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	DownTriggerData.TriggerTag = PRGameplayTags::Event_Ability_PlayerHitReact_Down;
	AbilityTriggers.Add(DownTriggerData);

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	bRetriggerInstancedAbility = true;
}

void UPRGA_PlayerHitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const EPRPlayerHitReactType HitReactType = ResolveHitReactType(TriggerEventData);
	if (HitReactType == EPRPlayerHitReactType::None)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHitReactFinished = false;
	ClearActionLock();
	ClearDownHitReact();
	ActiveHitReactType = HitReactType;
	ActivePlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	CancelActionsForHitReact(HitReactType);

	UAnimMontage* HitReactMontage = SelectHitReactMontage(HitReactType);
	if (!IsValid(HitReactMontage))
	{
		FinishHitReact(true);
		return;
	}
	if (HitReactType == EPRPlayerHitReactType::Down && !HasRequiredDownMontageSections(HitReactMontage))
	{
		FinishHitReact(true);
		return;
	}

	if (APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get())
	{
		if (HitReactType == EPRPlayerHitReactType::Strong || HitReactType == EPRPlayerHitReactType::Down)
		{
			// 강한 경직과 다운은 현재 이동을 즉시 끊어 행동불능 상태가 움직임과 섞이지 않게 한다.
			UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
			if (IsValid(CharacterMovement))
			{
				CharacterMovement->StopMovementImmediately();
			}
		}
	}

	StartActionLock(HitReactType);
	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		StartDownHitReact(ActivePlayerCharacter.Get());
	}

	ActiveHitReactMontage = HitReactMontage;
	const FName StartSection = HitReactType == EPRPlayerHitReactType::Down ? DownStartSectionName : NAME_None;
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		HitReactMontage,
		FMath::Max(MontagePlayRate, UE_SMALL_NUMBER),
		StartSection);

	if (!IsValid(ActiveMontageTask))
	{
		ClearDownHitReact();
		ClearActionLock();
		FinishHitReact(true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		ConfigureDownMontageFlow();
	}
}

void UPRGA_PlayerHitReact::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bHitReactFinished = true;
	ClearDownHitReact();
	ClearActionLock();

	if (IsValid(ActiveMontageTask))
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}

	if (IsValid(ActiveHitReactMontage))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			ASC->CurrentMontageStop(MontageStopBlendOutTime);
		}
		ActiveHitReactMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_PlayerHitReact::HandleHitReactMontageCompleted()
{
	ClearActionLock();
	FinishHitReact(false);
}

void UPRGA_PlayerHitReact::HandleHitReactMontageInterrupted()
{
	ClearActionLock();
	FinishHitReact(true);
}

void UPRGA_PlayerHitReact::HandlePlayerLanded(const FHitResult&)
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down || bHitReactFinished)
	{
		return;
	}

	bDownLandRequested = true;
	if (DownHitReactPhase == EPRPlayerDownHitReactPhase::FallLoop)
	{
		JumpToDownSection(DownLandSectionName);
	}
}

void UPRGA_PlayerHitReact::HandleDownMontageSectionChanged(UAnimMontage* Montage, FName SectionName, bool)
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down
		|| bHitReactFinished
		|| Montage != ActiveHitReactMontage.Get())
	{
		return;
	}

	if (SectionName == DownStartSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::Start;
		return;
	}

	if (SectionName == DownFallLoopSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::FallLoop;
		if (bDownLandRequested || IsPlayerMovingOnGround())
		{
			JumpToDownSection(DownLandSectionName);
		}
		return;
	}

	if (SectionName == DownLandSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::Land;
		bDownLandRequested = false;
		return;
	}

	if (SectionName == DownGetUpSectionName)
	{
		DownHitReactPhase = EPRPlayerDownHitReactPhase::GetUp;
	}
}

EPRPlayerHitReactType UPRGA_PlayerHitReact::ResolveHitReactType(const FGameplayEventData* TriggerEventData) const
{
	if (TriggerEventData == nullptr)
	{
		return EPRPlayerHitReactType::None;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Down))
	{
		return EPRPlayerHitReactType::Down;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Strong))
	{
		return EPRPlayerHitReactType::Strong;
	}

	if (TriggerEventData->EventTag.MatchesTagExact(PRGameplayTags::Event_Ability_PlayerHitReact_Weak))
	{
		return EPRPlayerHitReactType::Weak;
	}

	return EPRPlayerHitReactType::None;
}

UAnimMontage* UPRGA_PlayerHitReact::SelectHitReactMontage(EPRPlayerHitReactType HitReactType) const
{
	if (HitReactType == EPRPlayerHitReactType::Weak)
	{
		return SelectWeakHitReactMontage();
	}

	if (HitReactType == EPRPlayerHitReactType::Strong)
	{
		return SelectStrongHitReactMontage();
	}

	if (HitReactType == EPRPlayerHitReactType::Down)
	{
		return DownHitReactMontage.Get();
	}

	return nullptr;
}

UAnimMontage* UPRGA_PlayerHitReact::SelectWeakHitReactMontage() const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return UnarmedWeakHitReactMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		return UnarmedWeakHitReactMontage.Get();
	}

	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
	const UPRWeaponDataAsset* WeaponData = CurrentWeaponVisualInfo.WeaponData.Get();
	if (!IsValid(WeaponData))
	{
		WeaponData = WeaponManager->GetWeaponDataBySlotType(WeaponManager->GetCurrentWeaponSlot());
	}

	const EPRWeaponType WeaponType = IsValid(WeaponData) ? WeaponData->WeaponType : EPRWeaponType::None;
	if (const TObjectPtr<UAnimMontage>* FoundMontage = WeakHitReactMontages.Find(WeaponType))
	{
		UAnimMontage* SelectedMontage = FoundMontage->Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}

	return UnarmedWeakHitReactMontage.Get();
}

UAnimMontage* UPRGA_PlayerHitReact::SelectStrongHitReactMontage() const
{
	const APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter))
	{
		return UnarmedStrongHitReactMontage.Get();
	}

	const UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager();
	if (!IsValid(WeaponManager)
		|| WeaponManager->GetArmedState() == EPRArmedState::Unarmed
		|| WeaponManager->GetCurrentWeaponSlot() == EPRWeaponSlotType::None)
	{
		return UnarmedStrongHitReactMontage.Get();
	}

	const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
	const UPRWeaponDataAsset* WeaponData = CurrentWeaponVisualInfo.WeaponData.Get();
	if (!IsValid(WeaponData))
	{
		WeaponData = WeaponManager->GetWeaponDataBySlotType(WeaponManager->GetCurrentWeaponSlot());
	}

	const EPRWeaponType WeaponType = IsValid(WeaponData) ? WeaponData->WeaponType : EPRWeaponType::None;
	if (const TObjectPtr<UAnimMontage>* FoundMontage = WeakHitReactMontages.Find(WeaponType))
	{
		UAnimMontage* SelectedMontage = FoundMontage->Get();
		if (IsValid(SelectedMontage))
		{
			return SelectedMontage;
		}
	}

	return UnarmedStrongHitReactMontage.Get();
}

void UPRGA_PlayerHitReact::CancelActionsForHitReact(EPRPlayerHitReactType HitReactType)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(PRGameplayTags::Ability_Player_Reload);

	if (HitReactType == EPRPlayerHitReactType::Strong || HitReactType == EPRPlayerHitReactType::Down)
	{
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Weapon_Fire_Primary);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Aim);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Crouch);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Dodge);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Sprint);
		CancelTags.AddTag(PRGameplayTags::Ability_Player_Interaction);
	}

	ASC->CancelAbilities(&CancelTags, nullptr, this);
}

void UPRGA_PlayerHitReact::StartDownHitReact(APRPlayerCharacter* PlayerCharacter)
{
	DownHitReactPhase = EPRPlayerDownHitReactPhase::Start;
	bDownLandRequested = IsPlayerMovingOnGround();

	if (IsValid(PlayerCharacter))
	{
		PlayerLandedDelegateHandle = PlayerCharacter->OnPlayerLanded.AddUObject(
			this,
			&UPRGA_PlayerHitReact::HandlePlayerLanded);
	}
}

void UPRGA_PlayerHitReact::ConfigureDownMontageFlow()
{
	if (ActiveHitReactType != EPRPlayerHitReactType::Down)
	{
		return;
	}

	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage))
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_SetNextSection(DownStartSectionName, DownFallLoopSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownFallLoopSectionName, DownFallLoopSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownLandSectionName, DownGetUpSectionName, ActiveHitReactMontage);
	AnimInstance->Montage_SetNextSection(DownGetUpSectionName, NAME_None, ActiveHitReactMontage);

	FOnMontageSectionChanged SectionChangedDelegate;
	SectionChangedDelegate.BindUObject(this, &UPRGA_PlayerHitReact::HandleDownMontageSectionChanged);
	AnimInstance->Montage_SetSectionChangedDelegate(SectionChangedDelegate, ActiveHitReactMontage);
}

void UPRGA_PlayerHitReact::ClearDownHitReact()
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (PlayerLandedDelegateHandle.IsValid() && IsValid(PlayerCharacter))
	{
		PlayerCharacter->OnPlayerLanded.Remove(PlayerLandedDelegateHandle);
	}
	PlayerLandedDelegateHandle.Reset();

	if (IsValid(PlayerCharacter) && IsValid(ActiveHitReactMontage))
	{
		USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
		if (IsValid(MeshComponent))
		{
			UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
			if (IsValid(AnimInstance))
			{
				FOnMontageSectionChanged EmptyDelegate;
				AnimInstance->Montage_SetSectionChangedDelegate(EmptyDelegate, ActiveHitReactMontage);
			}
		}
	}

	ActivePlayerCharacter.Reset();
	ActiveHitReactType = EPRPlayerHitReactType::None;
	DownHitReactPhase = EPRPlayerDownHitReactPhase::None;
	bDownLandRequested = false;
}

bool UPRGA_PlayerHitReact::IsPlayerMovingOnGround() const
{
	const APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter))
	{
		return false;
	}

	const UCharacterMovementComponent* CharacterMovement = PlayerCharacter->GetCharacterMovement();
	return IsValid(CharacterMovement) && CharacterMovement->IsMovingOnGround();
}

void UPRGA_PlayerHitReact::JumpToDownSection(FName SectionName)
{
	APRPlayerCharacter* PlayerCharacter = ActivePlayerCharacter.Get();
	if (!IsValid(PlayerCharacter) || !IsValid(ActiveHitReactMontage) || SectionName.IsNone())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_JumpToSection(SectionName, ActiveHitReactMontage);
	}
}

bool UPRGA_PlayerHitReact::HasRequiredDownMontageSections(const UAnimMontage* Montage) const
{
	return IsValid(Montage)
		&& Montage->GetSectionIndex(DownStartSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownFallLoopSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownLandSectionName) != INDEX_NONE
		&& Montage->GetSectionIndex(DownGetUpSectionName) != INDEX_NONE;
}

void UPRGA_PlayerHitReact::StartActionLock(EPRPlayerHitReactType HitReactType)
{
	if (HitReactType != EPRPlayerHitReactType::Strong && HitReactType != EPRPlayerHitReactType::Down)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->AddLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
		ASC->AddReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
		bActionLockTagAdded = true;
	}
}

void UPRGA_PlayerHitReact::ClearActionLock()
{
	if (!bActionLockTagAdded)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->RemoveLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
		ASC->RemoveReplicatedLooseGameplayTag(PRGameplayTags::State_PlayerHitReactLocked);
	}

	bActionLockTagAdded = false;
}

void UPRGA_PlayerHitReact::FinishHitReact(bool bWasCancelled)
{
	if (bHitReactFinished)
	{
		return;
	}

	bHitReactFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
