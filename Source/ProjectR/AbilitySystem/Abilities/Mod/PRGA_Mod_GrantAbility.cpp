// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Mod_GrantAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/ItemSystem/Data/PRWeaponDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRWeaponModDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance_Weapon.h"

UPRGA_Mod_GrantAbility::UPRGA_Mod_GrantAbility()
{
	
}

void UPRGA_Mod_GrantAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayAbility::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UPRGA_Mod_GrantAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!IsValid(AbilityToGrant))
	{
		K2_EndAbility();
		return;
	}
	
	if (!HasAuthority(&ActivationInfo))
	{
		// 클라는 서버의 종료 신호를 대기 or Input 재입력 대기
		UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this,false);
		WaitInputPress->OnPress.AddDynamic(this,&ThisClass::OnInputPressed);
		WaitInputPress->ReadyForActivation();
		return;
	}
	
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		FGameplayAbilitySpec AbilitySpecToGrant (AbilityToGrant,1);
		AbilitySpecToGrant.SourceObject = this;
		GrantedSpecHandle = ASC->GiveAbility(AbilitySpecToGrant);
	}
	
	if (UPRWeaponDataAsset* WeaponData = GetCurrentWeaponData())
	{
		if (ModCostPolicy == EPRModCostPolicy::Stack)
		{
			WaitModStackExhausted(WeaponData->SlotType);	
		}
		else if (ModCostPolicy == EPRModCostPolicy::GaugeDuration)
		{
			// TODO: 게이지 소모 감지
		}
	}
}

void UPRGA_Mod_GrantAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (CostAttribute.IsValid() && CostExhaustedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).Remove(CostExhaustedHandle);
			CostExhaustedHandle.Reset();
		}
	
		if (GrantedSpecHandle.IsValid())
		{
			ASC->ClearAbility(GrantedSpecHandle);
			GrantedSpecHandle = FGameplayAbilitySpecHandle(); // Invalidate
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Mod_GrantAbility::WaitModStackExhausted(EPRWeaponSlotType WeaponSlotType)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp,Warning,TEXT("UPRGA_Mod_GrantAbility::WaitModStackExhausted: Invalid ASC. ObjectName: %s"),*GetName());
	}

	switch (WeaponSlotType)
	{
	case EPRWeaponSlotType::Primary:
		CostAttribute = UPRAttributeSet_Weapon::GetPrimaryModStackAttribute();
		break;
	case EPRWeaponSlotType::Secondary:
		CostAttribute = UPRAttributeSet_Weapon::GetSecondaryModStackAttribute();
		break;
	default:
		break;
	}
	
	if (CostAttribute.IsValid())
	{
		CostExhaustedHandle = ASC->GetGameplayAttributeValueChangeDelegate(CostAttribute).AddWeakLambda(
			this,[this](const FOnAttributeChangeData& AttributeChangeData)
		{
			if (FMath::IsNearlyZero(AttributeChangeData.NewValue))
			{
				K2_EndAbility();		
			}
		});	
	}
}

void UPRGA_Mod_GrantAbility::OnInputPressed(float TimeWaited)
{
	K2_EndAbility();
}
