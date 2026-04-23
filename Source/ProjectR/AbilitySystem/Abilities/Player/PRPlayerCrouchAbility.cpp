// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "ProjectR/AbilitySystem/Abilities/Player/PRPlayerCrouchAbility.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "ProjectR/PRGameplayTags.h"

UPRPlayerCrouchAbility::UPRPlayerCrouchAbility()
{
	// 태그 설정
	AbilityTags.AddTag(PRGameplayTags::Ability_Player_Crouch);
	ActivationOwnedTags.AddTag(PRGameplayTags::State_Crouching);
	InputTag = PRGameplayTags::Input_Ability_Crouch;

	// 네트워크 설정
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPRPlayerCrouchAbility::ActivateAbility(const FGameplayAbilitySpecHandle SpecHandle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(SpecHandle, ActorInfo, ActivationInfo, TriggerEventData);
	
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (IsLocallyControlled())
	{
		// 1. [클라이언트] 현재 위치 저장
		FVector_NetQuantize100 CurrentLoc = Character->GetActorLocation();

		// 서버 장이면 그냥 실행
		if (HasAuthority(&ActivationInfo))
		{
			bServerReceivedLocation = true;
			SavedLocation = CurrentLoc;
		}
		else
		{
			// 클라이언트면 서버에 위치 전달 RPC
			Server_SendCrouchLocation(CurrentLoc);
		}

		// 예측 실행
		ExecuteCrouch();
	}
	else // 서버 측
	{
		// 서버는 데이터가 도착할 때까지 대기하거나, 이미 도착했다면 실행
		if (bServerReceivedLocation)
		{
			ExecuteCrouch();
		}
	}
}

void UPRPlayerCrouchAbility::EndAbility(const FGameplayAbilitySpecHandle SpecHandle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (IsValid(Character))
	{
		Character->UnCrouch();
	}
	
	bServerReceivedLocation = false;
	Super::EndAbility(SpecHandle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRPlayerCrouchAbility::ExecuteCrouch()
{
	APRPlayerCharacter* Character = Cast<APRPlayerCharacter>(GetAvatarActorFromActorInfo());
	if (IsValid(Character) && Character->CanCrouch())
	{
		Character->Crouch();
		
		UAbilityTask_WaitInputPress* WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
		WaitInputTask->OnPress.AddDynamic(this, &UPRPlayerCrouchAbility::OnCrouchInputPressed);
		WaitInputTask->Activate();
	}
}

void UPRPlayerCrouchAbility::OnCrouchInputPressed(float TimeWaited)
{
	bool bReplicateEndAbility = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, false);
}

void UPRPlayerCrouchAbility::Server_SendCrouchLocation_Implementation(FVector_NetQuantize100 ClientLocation)
{
	bServerReceivedLocation = true;
	SavedLocation = ClientLocation;

	if (IsActive())
	{
		ExecuteCrouch();
	}
}
