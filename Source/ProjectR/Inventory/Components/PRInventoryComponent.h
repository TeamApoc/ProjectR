// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRInventoryComponent.generated.h"

class UPRItemInstance;
class UPRItemInstance_Weapon;
class UPRWeaponDataAsset;
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

	// 인벤토리 인덱스로 무기 Item을 조회한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRItemInstance_Weapon* GetWeaponItemAtIndex(int32 ItemIndex) const;

	// 식별자로 무기 Item을 조회한다
	UPRItemInstance_Weapon* FindWeaponByItemId(const FGuid& ItemId) const;

	// 인자로 받은 무기 Item을 현재 인벤토리가 소유하는지 확인한다
	bool OwnsWeapon(const UPRItemInstance_Weapon* WeaponItem) const;

protected:
	// 클라이언트에서 Item 목록 복제 결과를 확인한다
	UFUNCTION()
	void OnRep_ItemInstances();

	// 현재 인벤토리에 Item을 등록한다
	void RegisterReplicatedItem(UPRItemInstance* Item);

	// 현재 인벤토리에서 Item 등록을 해제한다
	void UnregisterReplicatedItem(UPRItemInstance* Item);

	// 클라이언트 요청을 서버의 Item 추가 처리로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_RequestAddWeaponItem(UPRWeaponDataAsset* WeaponData);

public:
	// 현재 인벤토리가 소유한 Item 목록
	UPROPERTY(ReplicatedUsing = OnRep_ItemInstances, VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Inventory")
	TArray<TObjectPtr<UPRItemInstance>> ItemInstances;
};
