// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRItemInstance_Mod.h"

#include "Logging/LogMacros.h"
#include "Net/UnrealNetwork.h"
#include "ProjectR/Weapon/Data/PRWeaponModDataAsset.h"

void UPRItemInstance_Mod::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPRItemInstance_Mod, ModData);
	DOREPLIFETIME(UPRItemInstance_Mod, EquippedWeaponItemId);
}

void UPRItemInstance_Mod::InitializeModItem(UPRWeaponModDataAsset* InModData)
{
	ModData = InModData;
	ClearEquippedWeaponItem();
}

bool UPRItemInstance_Mod::MatchesModData(const UPRWeaponModDataAsset* InModData) const
{
	return ModData == InModData;
}

bool UPRItemInstance_Mod::CanEquipToWeaponItem(const FGuid& WeaponItemId) const
{
	if (!WeaponItemId.IsValid())
	{
		return false;
	}

	return !IsEquipped() || EquippedWeaponItemId == WeaponItemId;
}

void UPRItemInstance_Mod::MarkEquippedToWeaponItem(const FGuid& WeaponItemId)
{
	if (!WeaponItemId.IsValid())
	{
		return;
	}

	EquippedWeaponItemId = WeaponItemId;
}

void UPRItemInstance_Mod::ClearEquippedWeaponItem()
{
	EquippedWeaponItemId.Invalidate();
}

void UPRItemInstance_Mod::OnRep_ModData()
{
	// 클라이언트에서 Mod 데이터 복제 결과를 ItemId 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item data replicated. ItemId=%s Mod=%s"),
		*GetItemId().ToString(),
		*GetNameSafe(ModData));
}

void UPRItemInstance_Mod::OnRep_EquippedWeaponItemId()
{
	// 클라이언트에서 Mod Item의 장착 대상 변경을 ItemId 기준으로 추적
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Inventory][Client] Mod item equipped weapon replicated. ItemId=%s Mod=%s EquippedWeaponItemId=%s"),
		*GetItemId().ToString(),
		*GetNameSafe(ModData),
		*EquippedWeaponItemId.ToString());
}
