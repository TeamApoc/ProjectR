// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRInventoryComponent.generated.h"

class UPRItemInstance;
class UPRItemInstance_Weapon;
class UPRWeaponDataAsset;

// 플레이어가 소유한 Item 인스턴스의 정본 컨테이너다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInventoryComponent();

public:
	// 새 무기 Item을 생성해 인벤토리에 추가한다
	UPRItemInstance_Weapon* AddWeaponItem(UPRWeaponDataAsset* WeaponData);

	// 식별자로 무기 Item을 조회한다
	UPRItemInstance_Weapon* FindWeaponByItemId(const FGuid& ItemId) const;

	// 인자로 받은 무기 Item을 현재 인벤토리가 소유하는지 확인한다
	bool OwnsWeapon(const UPRItemInstance_Weapon* WeaponItem) const;

protected:
	// 현재 인벤토리에 Item을 등록한다
	void RegisterReplicatedItem(UPRItemInstance* Item);

	// 현재 인벤토리에서 Item 등록을 해제한다
	void UnregisterReplicatedItem(UPRItemInstance* Item);

public:
	// 현재 인벤토리가 소유한 Item 목록
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TArray<TObjectPtr<UPRItemInstance>> ItemInstances;
};
