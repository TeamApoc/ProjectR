// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Inventory/Items/PRItemInstance.h"
#include "PRItemInstance_Mod.generated.h"

class UPRWeaponModDataAsset;

// 인벤토리가 소유하는 무기 Mod 1개의 지속 인스턴스다
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTR_API UPRItemInstance_Mod : public UPRItemInstance
{
	GENERATED_BODY()

public:
	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Mod 데이터를 연결한다
	void InitializeModItem(UPRWeaponModDataAsset* InModData);

	// 현재 연결된 Mod 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponModDataAsset* GetModData() const { return ModData; }

	// 입력 데이터가 현재 Mod와 같은지 확인한다
	bool MatchesModData(const UPRWeaponModDataAsset* InModData) const;

	// 현재 다른 무기 Item에 장착되어 있는지 확인한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	bool IsEquipped() const { return EquippedWeaponItemId.IsValid(); }

	// 현재 이 Mod를 장착 중인 무기 Item 식별자를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	FGuid GetEquippedWeaponItemId() const { return EquippedWeaponItemId; }

	// 지정 무기 Item에 장착 가능한 상태인지 확인한다
	bool CanEquipToWeaponItem(const FGuid& WeaponItemId) const;

	// 지정 무기 Item에 장착된 상태로 표시한다
	void MarkEquippedToWeaponItem(const FGuid& WeaponItemId);

	// 장착 중인 무기 Item 연결을 해제한다
	void ClearEquippedWeaponItem();

private:
	// Mod 데이터 복제 완료 시 클라이언트 확인 로그를 남긴다
	UFUNCTION()
	void OnRep_ModData();

	// 장착 대상 복제 완료 시 클라이언트 확인 로그를 남긴다
	UFUNCTION()
	void OnRep_EquippedWeaponItemId();

public:
	// 현재 연결된 Mod 데이터
	UPROPERTY(ReplicatedUsing = OnRep_ModData, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<UPRWeaponModDataAsset> ModData = nullptr;

	// 이 Mod를 장착 중인 무기 Item 식별자
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponItemId, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FGuid EquippedWeaponItemId;
};
