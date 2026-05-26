// Copyright ProjectR. All Rights Reserved.

#include "PRItemInstance_Equipment.h"
#include "ProjectR/ItemSystem/Data/PREquipmentDataAsset.h"
#include "ProjectR/ItemSystem/Components/PREquipmentManagerComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

void UPRItemInstance_Equipment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

bool UPRItemInstance_Equipment::ActivateItem(const FPRItemActivationContext& ActivationContext)
{
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->EquipItem(this);
}

bool UPRItemInstance_Equipment::DeactivateItem(const FPRItemActivationContext& ActivationContext)
{
	if (!IsValid(ActivationContext.UserActor) || !ActivationContext.UserActor->HasAuthority())
	{
		return false;
	}

	UPREquipmentManagerComponent* EquipmentManager = UPRGameplayStatics::GetEquipmentManagerComponent(ActivationContext.UserActor);
	if (!IsValid(EquipmentManager))
	{
		return false;
	}

	return EquipmentManager->UnequipSlot(GetSlotType());
}

UPREquipmentDataAsset* UPRItemInstance_Equipment::GetEquipmentData() const
{
	return Cast<UPREquipmentDataAsset>(ItemData);
}

EPREquipmentSlotType UPRItemInstance_Equipment::GetSlotType() const
{
	const UPREquipmentDataAsset* Data = GetEquipmentData();
	return IsValid(Data) ? Data->GetSlotType() : EPREquipmentSlotType::None;
}

void UPRItemInstance_Equipment::OnEquipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// [장착 로그] 
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 장착: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("[Equipment] 장착: %s"), *GetNameSafe(GetEquipmentData()));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Msg);
	}
}

void UPRItemInstance_Equipment::OnUnequipped(AActor* OwnerActor)
{
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// [해제 로그]
	UE_LOG(LogTemp, Log, TEXT("[Equipment] 해제: %s | 슬롯: %d | Owner: %s"), 
		*GetNameSafe(GetEquipmentData()), 
		(int32)GetSlotType(), 
		*GetNameSafe(OwnerActor));

	if (GEngine)
	{
		const FString Msg = FString::Printf(TEXT("[Equipment] 해제: %s"), *GetNameSafe(GetEquipmentData()));
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Msg);
	}
}

void UPRItemInstance_Equipment::FillSaveEntry(FPREquipmentSlotSaveEntry& OutEntry) const
{
	OutEntry = FPREquipmentSlotSaveEntry();
	OutEntry.SlotType = GetSlotType();
	// ItemIndex는 EquipmentManager 혹은 InventoryComponent에서 채운다.
}

void UPRItemInstance_Equipment::ApplySaveEntry(const FPREquipmentSlotSaveEntry& InEntry)
{
	// 인스턴스의 내부 상태가 필요해지면 복원
}
