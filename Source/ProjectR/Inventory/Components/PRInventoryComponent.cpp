// Copyright ProjectR. All Rights Reserved.

#include "PRInventoryComponent.h"

#include "Engine/ActorChannel.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
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

	return bWroteSomething;
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

	NewWeaponItem->InitializeWeaponItem(WeaponData);
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

	NewModItem->InitializeModItem(ModData);
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
	OnInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
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
	OnInventoryChanged(EPRInventoryChangeReason::ModEquipChanged);
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

void UPRInventoryComponent::OnInventoryChanged(EPRInventoryChangeReason ChangeReason)
{
	OnInventoryChangedDelegate.Broadcast(this, ChangeReason);
}

void UPRInventoryComponent::OnRep_InventoryWeaponItems()
{
	// 클라이언트에서 무기 Item 목록과 장착 Mod Item 참조 복제 결과를 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] InventoryWeaponItems replicated. Owner = %s | Count = %d"),
		*GetNameSafe(GetOwner()),
		InventoryWeaponItems.Num());

	for (int32 ItemIndex = 0; ItemIndex < InventoryWeaponItems.Num(); ++ItemIndex)
	{
		const UPRItemInstance_Weapon* WeaponItem = InventoryWeaponItems[ItemIndex];
		if (!IsValid(WeaponItem))
		{
			// 복제 배열에 비어 있는 무기 Item 항목이 포함된 상황을 인덱스 기준으로 추적
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Inventory][Client] WeaponItem[%d] Item=None"),
				ItemIndex);
			continue;
		}

		const UPRWeaponDataAsset* WeaponData = WeaponItem->GetWeaponData();
		// 무기 Item별 데이터와 Mod 연결 상태를 인덱스 기준으로 추적
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Client] Item[%d] WeaponItem. Item = %s | Weapon = %s | WeaponId = %s | Mod = %s | ModItem = %s"),
			ItemIndex,
			*GetNameSafe(WeaponItem),
			*GetNameSafe(WeaponData),
			IsValid(WeaponData) ? *WeaponData->GetDisplayName().ToString() : TEXT("None"),
			*GetNameSafe(WeaponItem->GetModData()),
			*GetNameSafe(WeaponItem->GetEquippedModItem()));
	}

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
}

void UPRInventoryComponent::OnRep_InventoryModItems()
{
	// 클라이언트에서 Mod Item 목록과 장착 대상 Item 참조 복제 결과를 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] InventoryModItems replicated. Owner = %s | Count = %d"),
		*GetNameSafe(GetOwner()),
		InventoryModItems.Num());

	for (int32 ItemIndex = 0; ItemIndex < InventoryModItems.Num(); ++ItemIndex)
	{
		const UPRItemInstance_Mod* ModItem = InventoryModItems[ItemIndex];
		if (!IsValid(ModItem))
		{
			// 복제 배열에 비어 있는 Mod Item 항목이 포함된 상황을 인덱스 기준으로 추적
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Inventory][Client] ModItem[%d] Item=None"),
				ItemIndex);
			continue;
		}

		const UPRWeaponModDataAsset* ModData = ModItem->GetModData();
		// Mod Item별 데이터와 장착 대상 Weapon Item 식별자를 인덱스 기준으로 추적
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Inventory][Client] ModItem[%d] Item = %s | Mod = %s | EquippedWeaponItem = %s"),
			ItemIndex,
			*GetNameSafe(ModItem),
			*GetNameSafe(ModData),
			*GetNameSafe(ModItem->GetEquippedWeaponItem()));
	}

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
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

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
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

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
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

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
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

	OnInventoryChanged(EPRInventoryChangeReason::ItemListChanged);
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

void UPRInventoryComponent::Server_RequestEquipModItemToWeapon_Implementation(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem)
{
	EquipModItemToWeapon(ModItem, TargetWeaponItem);
}

void UPRInventoryComponent::Server_RequestUnequipModFromWeapon_Implementation(UPRItemInstance_Weapon* TargetWeaponItem)
{
	UnequipModFromWeapon(TargetWeaponItem);
}
