// Copyright ProjectR. All Rights Reserved.

#include "PRInventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Inventory/Data/PRConsumableDataAsset.h"
#include "ProjectR/Inventory/Items/PRItemInstance_Consumable.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Mod.h"
#include "ProjectR/Weapon/Items/PRItemInstance_Weapon.h"

namespace
{
	// 무기와 Mod의 태그 호환 여부를 판정한다
	bool IsWeaponModCompatible(const UPRWeaponDataAsset* WeaponData, const UPRWeaponModDataAsset* ModData)
	{
		// 무기 데이터나 Mod 데이터가 없는 경우
		if (!IsValid(WeaponData) || !IsValid(ModData))
		{
			return false;
		}

		// Mod 태그가 비어 있으면 범용 Mod로 간주한다
		if (ModData->ModTags.IsEmpty())
		{
			return true;
		}

		// 무기가 지원 태그를 하나도 선언하지 않았으면 태그가 있는 Mod를 받을 수 없다
		if (WeaponData->SupportedModTags.IsEmpty())
		{
			return false;
		}

		return WeaponData->SupportedModTags.HasAny(ModData->ModTags);
	}
}

UPRInventoryComponent::UPRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRInventoryComponent, InventoryWeaponItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPRInventoryComponent, InventoryModItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPRInventoryComponent, InventoryConsumableItems, COND_OwnerOnly);
}

bool UPRInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (!RepFlags->bNetOwner)
	{
		return bWroteSomething;
	}

	for (UPRItemInstance_Weapon* WeaponItem : InventoryWeaponItems)
	{
		if (!IsValid(WeaponItem))
		{
			continue;
		}

		bWroteSomething |= Channel->ReplicateSubobject(WeaponItem, *Bunch, *RepFlags);
	}

	for (UPRItemInstance_Mod* ModItem : InventoryModItems)
	{
		if (!IsValid(ModItem))
		{
			continue;
		}

		bWroteSomething |= Channel->ReplicateSubobject(ModItem, *Bunch, *RepFlags);
	}

	for (UPRItemInstance_Consumable* ConsumableItem : InventoryConsumableItems)
	{
		if (!IsValid(ConsumableItem))
		{
			continue;
		}

		bWroteSomething |= Channel->ReplicateSubobject(ConsumableItem, *Bunch, *RepFlags);
	}

	return bWroteSomething;
}

void UPRInventoryComponent::RequestAddItem(UPRItemDataAsset* InItemData, int32 Amount)
{
	// 데이터가 없는 요청은 타입별 서버 요청으로 전달하지 않는다
	if (!IsValid(InItemData))
	{
		return;
	}

	if (UPRWeaponDataAsset* WeaponData = Cast<UPRWeaponDataAsset>(InItemData))
	{
		RequestAddWeaponItem(WeaponData);
	}
	else if (UPRWeaponModDataAsset* ModData = Cast<UPRWeaponModDataAsset>(InItemData))
	{
		RequestAddModItem(ModData);
	}
	else if (UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(InItemData))
	{
		RequestAddConsumableItem(ConsumableData, Amount);
	}
}

void UPRInventoryComponent::RequestAddWeaponItem(UPRWeaponDataAsset* WeaponData)
{
	// 데이터가 없는 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(WeaponData))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		AddWeaponItem(WeaponData);
		return;
	}

	Server_RequestAddWeaponItem(WeaponData);
}

UPRItemInstance* UPRInventoryComponent::AddItem(UPRItemDataAsset* InItemData, int32 Amount)
{
	// 데이터가 없는 요청은 Item 생성 대상으로 처리하지 않는다
	if (!IsValid(InItemData))
	{
		return nullptr;
	}

	if (UPRWeaponDataAsset* WeaponData = Cast<UPRWeaponDataAsset>(InItemData))
	{
		return AddWeaponItem(WeaponData);
	}
	else if (UPRWeaponModDataAsset* ModData = Cast<UPRWeaponModDataAsset>(InItemData))
	{
		return AddModItem(ModData);
	}
	else if (UPRConsumableDataAsset* ConsumableData = Cast<UPRConsumableDataAsset>(InItemData))
	{
		return AddConsumableItem(ConsumableData, Amount);
	}

	return nullptr;
}

UPRItemInstance_Weapon* UPRInventoryComponent::AddWeaponItem(UPRWeaponDataAsset* WeaponData)
{
	// 무기 정적 데이터가 없으면 Item 생성 자체를 진행하지 않는다
	if (!IsValid(WeaponData))
	{
		return nullptr;
	}

	UPRItemInstance_Weapon* NewWeaponItem = NewObject<UPRItemInstance_Weapon>(this);
	if (!IsValid(NewWeaponItem))
	{
		return nullptr;
	}

	NewWeaponItem->InitializeItem(WeaponData);
	RegisterInventoryWeaponItem(NewWeaponItem);

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Server] AddItem. Owner = %s | Item = %s | Weapon = %s | WeaponId = %s | Count = %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NewWeaponItem),
			*GetNameSafe(WeaponData),
			*WeaponData->GetDisplayName().ToString(),
			InventoryWeaponItems.Num());
	}

	return NewWeaponItem;
}

void UPRInventoryComponent::RequestAddModItem(UPRWeaponModDataAsset* ModData)
{
	// 데이터가 없는 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ModData))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		AddModItem(ModData);
		return;
	}

	Server_RequestAddModItem(ModData);
}

UPRItemInstance_Mod* UPRInventoryComponent::AddModItem(UPRWeaponModDataAsset* ModData)
{
	// Mod 정적 데이터가 없으면 Item 생성 자체를 진행하지 않는다
	if (!IsValid(ModData))
	{
		return nullptr;
	}

	UPRItemInstance_Mod* NewModItem = NewObject<UPRItemInstance_Mod>(this);
	if (!IsValid(NewModItem))
	{
		return nullptr;
	}

	NewModItem->InitializeItem(ModData,1);
	RegisterInventoryModItem(NewModItem);

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Server] AddModItem. Owner = %s | Item = %s | Mod = %s | Count = %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NewModItem),
			*GetNameSafe(ModData),
			InventoryModItems.Num());
	}

	return NewModItem;
}

void UPRInventoryComponent::RequestAddConsumableItem(UPRConsumableDataAsset* ConsumableData, int32 AddCount)
{
	// 데이터가 없거나 수량이 잘못된 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ConsumableData) || AddCount <= 0)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		AddConsumableItem(ConsumableData, AddCount);
		return;
	}

	Server_RequestAddConsumableItem(ConsumableData, AddCount);
}

UPRItemInstance_Consumable* UPRInventoryComponent::AddConsumableItem(UPRConsumableDataAsset* ConsumableData, int32 AddCount)
{
	// 소비 Item 정적 데이터가 없거나 수량이 잘못되면 Item 생성을 진행하지 않는다
	if (!IsValid(ConsumableData) || AddCount <= 0)
	{
		return nullptr;
	}

	if (UPRItemInstance_Consumable* ExistingItem = FindConsumableItemByData(ConsumableData))
	{
		ExistingItem->AddStack(AddCount);

		if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Inventory][Server] AddConsumableStack. Owner = %s | Item = %s | Consumable = %s | StackCount = %d"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ExistingItem),
				*GetNameSafe(ConsumableData),
				ExistingItem->GetStackCount());
		}

		return ExistingItem;
	}

	UPRItemInstance_Consumable* NewConsumableItem = NewObject<UPRItemInstance_Consumable>(this);
	if (!IsValid(NewConsumableItem))
	{
		return nullptr;
	}

	NewConsumableItem->InitializeItem(ConsumableData, AddCount);
	RegisterInventoryConsumableItem(NewConsumableItem);

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Server] AddConsumableItem. Owner = %s | Item = %s | Consumable = %s | StackCount = %d | Count = %d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NewConsumableItem),
			*GetNameSafe(ConsumableData),
			NewConsumableItem->GetStackCount(),
			InventoryConsumableItems.Num());
	}

	return NewConsumableItem;
}

void UPRInventoryComponent::RequestRemoveConsumableItem(UPRItemInstance_Consumable* ConsumableItem, int32 RemoveCount)
{
	// 잘못된 소비 Item 제거 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ConsumableItem) || RemoveCount <= 0)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		RemoveConsumableItemInternal(ConsumableItem, RemoveCount);
		return;
	}

	Server_RequestRemoveConsumableItem(ConsumableItem, RemoveCount);
}

void UPRInventoryComponent::RequestRemoveConsumableItemByData(UPRConsumableDataAsset* ConsumableData, int32 RemoveCount)
{
	// 잘못된 소비 Item 데이터 제거 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ConsumableData) || RemoveCount <= 0)
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		RemoveConsumableItemByDataInternal(ConsumableData, RemoveCount);
		return;
	}

	Server_RequestRemoveConsumableItemByData(ConsumableData, RemoveCount);
}

void UPRInventoryComponent::RequestUseConsumableItem(UPRItemInstance_Consumable* ConsumableItem, AActor* UserActor)
{
	// 잘못된 소비 Item 사용 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ConsumableItem) || !IsValid(UserActor))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UseConsumableItemInternal(ConsumableItem, UserActor);
		return;
	}

	Server_RequestUseConsumableItem(ConsumableItem, UserActor);
}

void UPRInventoryComponent::RequestUseConsumableItemByData(UPRConsumableDataAsset* ConsumableData, AActor* UserActor)
{
	// 잘못된 소비 Item 데이터 사용 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ConsumableData) || !IsValid(UserActor))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UseConsumableItemByDataInternal(ConsumableData, UserActor);
		return;
	}

	Server_RequestUseConsumableItemByData(ConsumableData, UserActor);
}

bool UPRInventoryComponent::RemoveConsumableItemInternal(UPRItemInstance_Consumable* ConsumableItem, int32 RemoveCount)
{
	// 소비 Item 제거는 인벤토리가 소유한 인스턴스만 대상으로 처리한다
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ConsumableItem) || !OwnsConsumable(ConsumableItem) || RemoveCount <= 0)
	{
		return false;
	}

	const int32 PreviousStackCount = ConsumableItem->GetStackCount();
	if (!ConsumableItem->RemoveStack(RemoveCount))
	{
		return false;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] RemoveConsumableItem. Owner = %s | Item = %s | Consumable = %s | BeforeStackCount = %d | AfterStackCount = %d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(ConsumableItem),
		*GetNameSafe(ConsumableItem->GetConsumableData()),
		PreviousStackCount,
		ConsumableItem->GetStackCount());

	if (ConsumableItem->GetStackCount() <= 0)
	{
		UnregisterConsumableItemInstance(ConsumableItem);
	}

	return true;
}

bool UPRInventoryComponent::RemoveConsumableItemByDataInternal(UPRConsumableDataAsset* ConsumableData, int32 RemoveCount)
{
	// 데이터 기반 제거는 현재 인벤토리의 소비 Item 인스턴스를 먼저 찾는다
	if (!IsValid(ConsumableData))
	{
		return false;
	}

	return RemoveConsumableItemInternal(FindConsumableItemByData(ConsumableData), RemoveCount);
}

bool UPRInventoryComponent::UseConsumableItemInternal(UPRItemInstance_Consumable* ConsumableItem, AActor* UserActor)
{
	// 소비 Item 사용은 인벤토리가 소유한 인스턴스만 대상으로 처리한다
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(ConsumableItem) || !OwnsConsumable(ConsumableItem) || !IsValid(UserActor))
	{
		return false;
	}

	if (!ConsumableItem->UseItem(UserActor))
	{
		return false;
	}

	if (ConsumableItem->GetStackCount() <= 0)
	{
		UnregisterConsumableItemInstance(ConsumableItem);
	}

	return true;
}

bool UPRInventoryComponent::UseConsumableItemByDataInternal(UPRConsumableDataAsset* ConsumableData, AActor* UserActor)
{
	// 데이터 기반 사용은 현재 인벤토리의 소비 Item 인스턴스를 먼저 찾는다
	if (!IsValid(ConsumableData))
	{
		return false;
	}

	return UseConsumableItemInternal(FindConsumableItemByData(ConsumableData), UserActor);
}

void UPRInventoryComponent::RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 잘못된 Item 참조 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(ModItem) || !IsValid(TargetWeaponItem))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		EquipModItemToWeapon(ModItem, TargetWeaponItem);
		return;
	}

	Server_RequestEquipModItemToWeapon(ModItem, TargetWeaponItem);
}

void UPRInventoryComponent::RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 잘못된 Item 참조 요청은 서버 RPC를 보내기 전에 중단한다
	if (!IsValid(TargetWeaponItem))
	{
		return;
	}

	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		UnequipModFromWeapon(TargetWeaponItem);
		return;
	}

	Server_RequestUnequipModFromWeapon(TargetWeaponItem);
}

bool UPRInventoryComponent::EquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 서버 내부 장착에 필요한 Item 참조와 소유권이 유효하지 않은 경우
	if (!IsValid(ModItem) || !OwnsMod(ModItem) || !IsValid(ModItem->GetModData()))
	{
		// Mod Item 소유권이나 데이터 누락 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | ModItem = %s | Reason = InvalidModItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ModItem));

		// 장착 실패. Mod Item 조회나 데이터 검증 실패
		return false;
	}

	// 장착 대상 Weapon Item은 같은 인벤토리 소유여야 서버 정본 변경이 가능하다
	if (!IsValid(TargetWeaponItem) || !OwnsWeapon(TargetWeaponItem))
	{
		// Weapon Item 소유권 검증 실패 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = InvalidWeaponItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 장착 실패. Weapon Item 소유권 검증 실패
		return false;
	}

	// Mod 장착은 인벤토리 정본 변경 전 무기 데이터와 Mod 태그 호환성을 먼저 검증한다
	if (!IsWeaponModCompatible(TargetWeaponItem->GetWeaponData(), ModItem->GetModData()))
	{
		// 호환되지 않는 무기와 Mod 조합을 Owner, Weapon Item, Mod Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Reason = IncompatibleMod"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem),
			*GetNameSafe(ModItem));

		// 장착 실패. 무기와 Mod 태그 호환성 검증 실패
		return false;
	}

	// 이미 같은 무기에 같은 Mod Item이 연결된 경우 중복 요청으로 처리한다
	if (TargetWeaponItem->GetEquippedModItem() == ModItem
		&& ModItem->GetEquippedWeaponItem() == TargetWeaponItem)
	{
		// 중복 장착 요청을 Owner, Weapon Item, Mod Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 장착 실패. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Reason = AlreadyEquipped"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem),
			*GetNameSafe(ModItem));

		// 장착 실패. 이미 동일한 연결 상태다
		return false;
	}

	// 이동 장착을 위해 Mod Item이 기억하던 기존 Weapon Item 연결을 먼저 해제한다
	UPRItemInstance_Weapon* PreviousWeaponItem = ModItem->GetEquippedWeaponItem();
	if (IsValid(PreviousWeaponItem) && OwnsWeapon(PreviousWeaponItem) && PreviousWeaponItem != TargetWeaponItem)
	{
		// 기존 장착 무기에서 Mod Item 연결과 적용 데이터를 제거한다
		PreviousWeaponItem->ClearEquippedModItem();
		PreviousWeaponItem->SetModData(nullptr);

		// 기존 무기가 현재 장착 중이면 어빌리티와 공개 상태를 해제 상태로 갱신한다
		NotifyWeaponItemModChanged(PreviousWeaponItem);
	}
	else if (!IsValid(PreviousWeaponItem) || !OwnsWeapon(PreviousWeaponItem))
	{
		// 인벤토리에 없는 Weapon Item을 가리키는 오래된 연결은 새 장착 전에 정리한다
		ModItem->ClearEquippedWeaponItem();
	}

	// 타겟 무기에 다른 Mod Item이 있었다면 해당 Mod Item의 역참조를 먼저 비운다
	UPRItemInstance_Mod* PreviousModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(PreviousModItem) && PreviousModItem != ModItem)
	{
		PreviousModItem->ClearEquippedWeaponItem();
	}

	// 인벤토리 정본 상태를 새 Mod Item과 타겟 Weapon Item의 양방향 연결로 확정한다
	TargetWeaponItem->SetEquippedModItem(ModItem);
	TargetWeaponItem->SetModData(ModItem->GetModData());
	ModItem->MarkEquippedToWeaponItem(TargetWeaponItem);

	// 타겟 무기가 현재 장착 중이면 어빌리티와 공개 상태를 새 Mod 기준으로 갱신한다
	NotifyWeaponItemModChanged(TargetWeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	// 서버 장착 처리가 확정된 뒤 Owner, Weapon Item, Mod Item, Mod 데이터를 남겨 UI 요청 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] Mod 장착 완료. EquipModItemToWeapon() | Owner = %s | TargetWeaponItem = %s | ModItem = %s | Mod = %s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetWeaponItem),
		*GetNameSafe(ModItem),
		*GetNameSafe(ModItem->GetModData()));

	// 장착 성공. 기존 연결 해제, 새 연결 확정, 장착 중 무기 런타임 갱신 마침
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ModEquipChanged;
	EventData.ItemInstance = TargetWeaponItem->GetEquippedModItem();
	OnInventoryChanged(EventData);
	return true;
}

bool UPRInventoryComponent::UnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem)
{
	// 해제 대상 Weapon Item은 같은 인벤토리 소유여야 서버 정본 변경이 가능하다
	if (!IsValid(TargetWeaponItem) || !OwnsWeapon(TargetWeaponItem))
	{
		// Weapon Item 소유권 검증 실패 상황을 Owner와 Item 참조 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 해제 실패. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = InvalidWeaponItem"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 해제 실패. Weapon Item 소유권 검증 실패
		return false;
	}

	// Weapon Item이 기억하던 Mod Item이 있으면 Mod Item의 역참조를 함께 정리한다
	UPRItemInstance_Mod* PreviousModItem = TargetWeaponItem->GetEquippedModItem();
	if (IsValid(PreviousModItem))
	{
		PreviousModItem->ClearEquippedWeaponItem();
	}

	// 장착된 Mod Item도 Mod 데이터도 없으면 해제할 상태가 없다
	if (!IsValid(PreviousModItem) && !IsValid(TargetWeaponItem->GetModData()))
	{
		// 무변경 해제 요청을 Owner와 Weapon Item 기준으로 추적
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Inventory][Server] Mod 해제 실패. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | Reason = EmptyMod"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetWeaponItem));

		// 해제 실패. 이미 빈 Mod 상태다
		return false;
	}

	// 인벤토리 정본 상태에서 Weapon Item의 Mod 연결을 제거한다
	TargetWeaponItem->ClearEquippedModItem();
	TargetWeaponItem->SetModData(nullptr);

	// 타겟 무기가 현재 장착 중이면 어빌리티와 공개 상태를 빈 Mod 기준으로 갱신한다
	NotifyWeaponItemModChanged(TargetWeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	// 서버 해제 처리가 확정된 뒤 Owner, Weapon Item, 이전 Mod Item을 남겨 UI 요청 흐름 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Server] Mod 해제 완료. UnequipModFromWeapon() | Owner = %s | TargetWeaponItem = %s | PreviousModItem = %s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetWeaponItem),
		*GetNameSafe(PreviousModItem));

	// 해제 성공. Mod Item 역참조와 Weapon Item Mod 상태 갱신 마침
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ModEquipChanged;
	EventData.ItemInstance = TargetWeaponItem->GetEquippedModItem();
	
	OnInventoryChanged(EventData);
	return true;
}

UPRItemInstance_Weapon* UPRInventoryComponent::GetWeaponItemAtIndex(int32 ItemIndex) const
{
	if (!InventoryWeaponItems.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	return InventoryWeaponItems[ItemIndex];
}

UPRItemInstance_Mod* UPRInventoryComponent::GetModItemAtIndex(int32 ItemIndex) const
{
	if (!InventoryModItems.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	return InventoryModItems[ItemIndex];
}

UPRItemInstance_Consumable* UPRInventoryComponent::GetConsumableItemAtIndex(int32 ItemIndex) const
{
	if (!InventoryConsumableItems.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	return InventoryConsumableItems[ItemIndex];
}

bool UPRInventoryComponent::OwnsWeapon(const UPRItemInstance_Weapon* WeaponItem) const
{
	// 유효하지 않은 Item은 소유권 검사에서 즉시 실패한다
	if (!IsValid(WeaponItem))
	{
		return false;
	}

	return InventoryWeaponItems.Contains(WeaponItem);
}

bool UPRInventoryComponent::OwnsMod(const UPRItemInstance_Mod* ModItem) const
{
	// 유효하지 않은 Item은 소유권 검사에서 즉시 실패한다
	if (!IsValid(ModItem))
	{
		return false;
	}

	return InventoryModItems.Contains(ModItem);
}

bool UPRInventoryComponent::OwnsConsumable(const UPRItemInstance_Consumable* ConsumableItem) const
{
	// 유효하지 않은 Item은 소유권 검사에서 즉시 실패한다
	if (!IsValid(ConsumableItem))
	{
		return false;
	}

	return InventoryConsumableItems.Contains(ConsumableItem);
}

UPRItemInstance_Consumable* UPRInventoryComponent::FindConsumableItemByData(const UPRConsumableDataAsset* ConsumableData) const
{
	// 유효하지 않은 데이터는 조회 대상이 아니다
	if (!IsValid(ConsumableData))
	{
		return nullptr;
	}

	for (UPRItemInstance_Consumable* ConsumableItem : InventoryConsumableItems)
	{
		if (!IsValid(ConsumableItem))
		{
			continue;
		}

		if (ConsumableItem->GetConsumableData() == ConsumableData)
		{
			return ConsumableItem;
		}
	}

	return nullptr;
}

UPRItemInstance_Weapon* UPRInventoryComponent::FindWeaponItemByData(const UPRWeaponDataAsset* WeaponData)
{
	// 유효하지 않은 데이터는 조회 대상이 아니다
	if (!IsValid(WeaponData))
	{
		return nullptr;
	}

	for (UPRItemInstance_Weapon* WeaponItem : InventoryWeaponItems)
	{
		if (!IsValid(WeaponItem))
		{
			continue;
		}

		if (WeaponItem->GetWeaponData() == WeaponData)
		{
			return WeaponItem;
		}
	}

	return nullptr;
}

UPRItemInstance_Mod* UPRInventoryComponent::FindModItemByData(const UPRWeaponModDataAsset* ItemData)
{
	// 유효하지 않은 데이터는 조회 대상이 아니다
	if (!IsValid(ItemData))
	{
		return nullptr;
	}

	for (UPRItemInstance_Mod* ModItem : InventoryModItems)
	{
		if (!IsValid(ModItem))
		{
			continue;
		}

		if (ModItem->GetModData() == ItemData)
		{
			return ModItem;
		}
	}

	return nullptr;
}

void UPRInventoryComponent::OnInventoryChanged(const FPRInventoryChangeEventData& EventData)
{
	OnInventoryChangedDelegate.Broadcast(this, EventData);
}

void UPRInventoryComponent::OnRep_InventoryWeaponItems(const TArray<UPRItemInstance_Weapon*>& OldWeaponItems)
{
	// 새로 추가된 무기 Item마다 ItemAdded 이벤트 발행
	for (UPRItemInstance_Weapon* WeaponItem : InventoryWeaponItems)
	{
		if (!IsValid(WeaponItem) || OldWeaponItems.Contains(WeaponItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
		EventData.ItemInstance = WeaponItem;

		OnInventoryChanged(EventData);
	}

	// 제거된 무기 Item마다 ItemRemoved 이벤트 발행
	for (UPRItemInstance_Weapon* OldWeaponItem : OldWeaponItems)
	{
		if (!IsValid(OldWeaponItem) || InventoryWeaponItems.Contains(OldWeaponItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
		EventData.ItemInstance = OldWeaponItem;

		OnInventoryChanged(EventData);
	}
}

void UPRInventoryComponent::OnRep_InventoryModItems(const TArray<UPRItemInstance_Mod*>& OldModItems)
{
	// 새로 추가된 Mod Item마다 ItemAdded 이벤트 발행
	for (UPRItemInstance_Mod* ModItem : InventoryModItems)
	{
		if (!IsValid(ModItem) || OldModItems.Contains(ModItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
		EventData.ItemInstance = ModItem;

		OnInventoryChanged(EventData);
	}

	// 제거된 Mod Item마다 ItemRemoved 이벤트 발행
	for (UPRItemInstance_Mod* OldModItem : OldModItems)
	{
		if (!IsValid(OldModItem) || InventoryModItems.Contains(OldModItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
		EventData.ItemInstance = OldModItem;

		OnInventoryChanged(EventData);
	}
}

void UPRInventoryComponent::OnRep_InventoryConsumableItems(const TArray<UPRItemInstance_Consumable*>& OldConsumables)
{
	// 새로 추가된 소비 Item마다 ItemAdded 이벤트 발행
	for (UPRItemInstance_Consumable* ConsumableItem : InventoryConsumableItems)
	{
		if (!IsValid(ConsumableItem) || OldConsumables.Contains(ConsumableItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
		EventData.ItemInstance = ConsumableItem;

		OnInventoryChanged(EventData);
	}

	// 제거된 소비 Item마다 ItemRemoved 이벤트 발행
	for (UPRItemInstance_Consumable* OldConsumableItem : OldConsumables)
	{
		if (!IsValid(OldConsumableItem) || InventoryConsumableItems.Contains(OldConsumableItem))
		{
			continue;
		}

		FPRInventoryChangeEventData EventData;
		EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
		EventData.ItemInstance = OldConsumableItem;

		OnInventoryChanged(EventData);
	}
}

void UPRInventoryComponent::RegisterInventoryWeaponItem(UPRItemInstance_Weapon* WeaponItem)
{
	// 인벤토리에 실제로 들어갈 무기 Item만 목록에 등록한다
	if (!IsValid(WeaponItem))
	{
		return;
	}

	InventoryWeaponItems.AddUnique(WeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
	EventData.ItemInstance = WeaponItem;
	
	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::UnregisterInventoryWeaponItem(UPRItemInstance_Weapon* WeaponItem)
{
	// 잘못된 무기 Item 요청이면 등록 해제를 시도하지 않는다
	if (!IsValid(WeaponItem))
	{
		return;
	}

	InventoryWeaponItems.Remove(WeaponItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}
	
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
	EventData.ItemInstance = WeaponItem;

	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::RegisterInventoryModItem(UPRItemInstance_Mod* ModItem)
{
	// 인벤토리에 실제로 들어갈 무기 Mod Item만 목록에 등록한다
	if (!IsValid(ModItem))
	{
		return;
	}

	InventoryModItems.AddUnique(ModItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}
	
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
	EventData.ItemInstance = ModItem;

	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::UnregisterInventoryModItem(UPRItemInstance_Mod* ModItem)
{
	// 잘못된 무기 Mod Item 요청이면 등록 해제를 시도하지 않는다
	if (!IsValid(ModItem))
	{
		return;
	}

	InventoryModItems.Remove(ModItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
	EventData.ItemInstance = ModItem;
	
	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::RegisterInventoryConsumableItem(UPRItemInstance_Consumable* ConsumableItem)
{
	// 인벤토리에 실제로 들어갈 소비 Item만 목록에 등록한다
	if (!IsValid(ConsumableItem))
	{
		return;
	}

	InventoryConsumableItems.AddUnique(ConsumableItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}
	
	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemAdded;
	EventData.ItemInstance = ConsumableItem;
	
	OnInventoryChanged(EventData);
}

void UPRInventoryComponent::UnregisterConsumableItemInstance(UPRItemInstance_Consumable* ConsumableItem)
{
	// 잘못된 소비 Item 요청이면 등록 해제를 시도하지 않는다
	if (!IsValid(ConsumableItem))
	{
		return;
	}

	InventoryConsumableItems.Remove(ConsumableItem);

	if (IsValid(GetOwner()))
	{
		GetOwner()->ForceNetUpdate();
	}

	FPRInventoryChangeEventData EventData;
	EventData.ChangeReason = EPRInventoryChangeReason::ItemRemoved;
	EventData.ItemInstance = ConsumableItem;
	
	OnInventoryChanged(EventData);
}

UPRWeaponManagerComponent* UPRInventoryComponent::ResolveOwnerWeaponManager() const
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return nullptr;
	}

	AController* OwnerController = Cast<AController>(OwnerActor->GetOwner());
	if (!IsValid(OwnerController))
	{
		return nullptr;
	}

	APawn* OwnerPawn = OwnerController->GetPawn();
	if (!IsValid(OwnerPawn))
	{
		return nullptr;
	}

	return OwnerPawn->FindComponentByClass<UPRWeaponManagerComponent>();
}

void UPRInventoryComponent::NotifyWeaponItemModChanged(UPRItemInstance_Weapon* WeaponItem)
{
	// 잘못된 Weapon Item 요청이면 런타임 반응을 전달하지 않는다
	if (!IsValid(WeaponItem))
	{
		return;
	}

	UPRWeaponManagerComponent* WeaponManager = ResolveOwnerWeaponManager();
	if (!IsValid(WeaponManager))
	{
		return;
	}

	// WeaponManager는 현재 장착 중인 무기만 반응하고 인벤토리 정본 변경은 관여하지 않는다
	WeaponManager->HandleInventoryWeaponModChanged(WeaponItem);
}

void UPRInventoryComponent::Server_RequestAddWeaponItem_Implementation(UPRWeaponDataAsset* WeaponData)
{
	AddWeaponItem(WeaponData);
}

void UPRInventoryComponent::Server_RequestAddModItem_Implementation(UPRWeaponModDataAsset* ModData)
{
	AddModItem(ModData);
}

void UPRInventoryComponent::Server_RequestAddConsumableItem_Implementation(UPRConsumableDataAsset* ConsumableData, int32 AddCount)
{
	AddConsumableItem(ConsumableData, AddCount);
}

void UPRInventoryComponent::Server_RequestRemoveConsumableItem_Implementation(UPRItemInstance_Consumable* ConsumableItem, int32 RemoveCount)
{
	RemoveConsumableItemInternal(ConsumableItem, RemoveCount);
}

void UPRInventoryComponent::Server_RequestRemoveConsumableItemByData_Implementation(UPRConsumableDataAsset* ConsumableData, int32 RemoveCount)
{
	RemoveConsumableItemByDataInternal(ConsumableData, RemoveCount);
}

void UPRInventoryComponent::Server_RequestUseConsumableItem_Implementation(UPRItemInstance_Consumable* ConsumableItem, AActor* UserActor)
{
	UseConsumableItemInternal(ConsumableItem, UserActor);
}

void UPRInventoryComponent::Server_RequestUseConsumableItemByData_Implementation(UPRConsumableDataAsset* ConsumableData, AActor* UserActor)
{
	UseConsumableItemByDataInternal(ConsumableData, UserActor);
}

void UPRInventoryComponent::Server_RequestEquipModItemToWeapon_Implementation(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	EquipModItemToWeapon(ModItem, TargetWeaponItem);
}

void UPRInventoryComponent::Server_RequestUnequipModFromWeapon_Implementation(UPRItemInstance_Weapon* TargetWeaponItem)
{
	UnequipModFromWeapon(TargetWeaponItem);
}
