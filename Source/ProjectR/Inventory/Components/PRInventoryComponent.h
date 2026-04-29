// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRInventoryComponent.generated.h"

class UPRItemInstance_Mod;
class UPRItemInstance_Weapon;
class UPRWeaponManagerComponent;
class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;
class UActorChannel;
class FOutBunch;
struct FReplicationFlags;

// 플레이어가 소유한 Item 인스턴스의 정본 컨테이너다
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRInventoryComponent();

	/*~ UActorComponent Interface ~*/
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

public:
	// 무기 Item 추가를 서버 권위 경로로 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestAddWeaponItem(UPRWeaponDataAsset* WeaponData);

	// 새 무기 Item을 생성해 인벤토리에 추가한다
	UPRItemInstance_Weapon* AddWeaponItem(UPRWeaponDataAsset* WeaponData);

	// 무기 Mod Item 추가를 서버 권위 경로로 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestAddModItem(UPRWeaponModDataAsset* ModData);

	// 새 무기 Mod Item을 생성해 인벤토리에 추가한다
	UPRItemInstance_Mod* AddModItem(UPRWeaponModDataAsset* ModData);

	// Mod Item을 지정 무기 Item에 장착하도록 서버 권위 경로로 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 지정 무기 Item의 Mod Item 장착을 해제하도록 서버 권위 경로로 요청한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Inventory")
	void RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

	// Mod Item을 지정 무기 Item에 장착한다
	bool EquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 지정 무기 Item의 Mod Item 장착을 해제한다
	bool UnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

	// 인벤토리 인덱스로 무기 Item을 조회한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRItemInstance_Weapon* GetWeaponItemAtIndex(int32 ItemIndex) const;

	// 인벤토리 인덱스로 무기 Mod Item을 조회한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRItemInstance_Mod* GetModItemAtIndex(int32 ItemIndex) const;

	// 인자로 받은 무기 Item을 현재 인벤토리가 소유하는지 확인한다
	bool OwnsWeapon(const UPRItemInstance_Weapon* WeaponItem) const;

	// 인자로 받은 무기 Mod Item을 현재 인벤토리가 소유하는지 확인한다
	bool OwnsMod(const UPRItemInstance_Mod* ModItem) const;

protected:
	// 클라이언트에서 무기 Item 목록 복제 결과를 확인한다
	UFUNCTION()
	void OnRep_InventoryWeaponItems();

	// 클라이언트에서 무기 Mod Item 목록 복제 결과를 확인한다
	UFUNCTION()
	void OnRep_InventoryModItems();

	// 현재 인벤토리에 무기 Item을 등록한다
	void RegisterInventoryWeaponItem(UPRItemInstance_Weapon* WeaponItem);

	// 현재 인벤토리에서 무기 Item 등록을 해제한다
	void UnregisterInventoryWeaponItem(UPRItemInstance_Weapon* WeaponItem);

	// 현재 인벤토리에 무기 Mod Item을 등록한다
	void RegisterInventoryModItem(UPRItemInstance_Mod* ModItem);

	// 현재 인벤토리에서 무기 Mod Item 등록을 해제한다
	void UnregisterInventoryModItem(UPRItemInstance_Mod* ModItem);

	// 클라이언트 요청을 서버의 Item 추가 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestAddWeaponItem(UPRWeaponDataAsset* WeaponData);

	// 클라이언트 요청을 서버의 무기 Mod Item 추가 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestAddModItem(UPRWeaponModDataAsset* ModData);

	// 클라이언트 Mod 장착 요청을 서버의 인벤토리 장착 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestEquipModItemToWeapon(UPRItemInstance_Mod* ModItem, UPRItemInstance_Weapon* TargetWeaponItem);

	// 클라이언트 Mod 해제 요청을 서버의 인벤토리 해제 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestUnequipModFromWeapon(UPRItemInstance_Weapon* TargetWeaponItem);

private:
	// 현재 인벤토리 소유자의 무기 매니저 컴포넌트를 조회한다
	UPRWeaponManagerComponent* ResolveOwnerWeaponManager() const;

	// 현재 장착 중인 무기라면 런타임 Mod 변경 반응을 전달한다
	void NotifyWeaponItemModChanged(UPRItemInstance_Weapon* WeaponItem);

public:
	// 현재 인벤토리가 소유한 무기 Item 목록
	UPROPERTY(ReplicatedUsing = OnRep_InventoryWeaponItems, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TArray<TObjectPtr<UPRItemInstance_Weapon>> InventoryWeaponItems;

	// 현재 인벤토리가 소유한 무기 Mod Item 목록
	UPROPERTY(ReplicatedUsing = OnRep_InventoryModItems, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TArray<TObjectPtr<UPRItemInstance_Mod>> InventoryModItems;
};
