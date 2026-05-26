// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRItemInstance_Consumable.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "ProjectR/AbilitySystem/Abilities/Player/Consumable/PRGA_UseConsumable.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRConsumableDataAsset.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRItemInstance_Consumable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UPRItemInstance_Consumable::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	return UseItem(ActivationContext.UserActor);
}

bool UPRItemInstance_Consumable::UseItem(AActor* UserActor)
{
	const UPRConsumableDataAsset* ConsumableData = GetConsumableData();
	
	if (!IsValid(UserActor) || !UserActor->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = InvalidAuthorityOrUser"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	if (!CanUseItem(UserActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = CannotUse"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	UAbilitySystemComponent* ASC = UPRGameplayStatics::GetAbilitySystemComponent(UserActor);
	if (!IsValid(ASC) || !IsValid(ConsumableData->UseAbilityClass))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = InvalidAbility"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = UserActor;
	EventData.Target = UserActor;
	EventData.OptionalObject = this;

	FGameplayAbilitySpec AbilitySpec(ConsumableData->UseAbilityClass, 1, INDEX_NONE, this);
	const FGameplayAbilitySpecHandle AbilityHandle = ASC->GiveAbilityAndActivateOnce(AbilitySpec, &EventData);
	if (!AbilityHandle.IsValid())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ConsumableItem][Server] 사용 실패. UseItem() | Owner = %s | User = %s | Item = %s | Count = %d | Reason = ActivateAbilityFailed"),
			*GetNameSafe(GetTypedOuter<UPRInventoryComponent>()),
			*GetNameSafe(UserActor),
			*GetNameSafe(ConsumableData),
			StackCount);
		return false;
	}
	// 스택 소모는 Use Consumable 어빌리티 내에서 관리 

	return true;
}

UPRConsumableDataAsset* UPRItemInstance_Consumable::GetConsumableData() const
{
	return Cast<UPRConsumableDataAsset>(ItemData);
}

bool UPRItemInstance_Consumable::CanUseItem(AActor* UserActor) const
{
	const UPRConsumableDataAsset* ConsumableData = GetConsumableData();
	return IsValid(UserActor) && IsValid(ConsumableData) && IsValid(ConsumableData->UseAbilityClass) && StackCount > 0;
}
