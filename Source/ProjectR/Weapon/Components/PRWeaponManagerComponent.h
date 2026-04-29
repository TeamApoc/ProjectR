// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponManagerComponent.generated.h"

class APRWeaponActor;
class UAnimInstance;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Player;
class UPRInventoryComponent;
class UPRItemInstance_Weapon;
class UPRWeaponDataAsset;
class UPRWeaponModDataAsset;

// 캐릭터 기준 무기 장착 공개 상태와 슬롯별 로컬 Actor 생명주기를 관리하는 허브다.
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 무기 장착 허브 컴포넌트를 초기화한다
	UPRWeaponManagerComponent();

	/*~ UActorComponent Interface ~*/
	// 종료 시점에 슬롯별 로컬 Actor를 정리한다
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 복제할 공개 상태를 등록한다
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 현재 활성 무기 슬롯을 반환한다
	EPRWeaponSlotType GetCurrentWeaponSlot() const { return CurrentWeaponSlot; }

	// 현재 활성 무기의 공개 비주얼 정보를 반환한다
	const FPRWeaponVisualInfo& GetCurrentWeaponVisualInfo() const;

	// 현재 무장 상태를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	EPRArmedState GetArmedState() const { return ArmedState; }

	// PlayerState와 ASC, 인벤토리 연결 캐시를 초기화한다
	void InitializeRuntimeLinks();

	// 대상 슬롯에 연결된 무기 Item 원본을 반환한다
	UPRItemInstance_Weapon* GetWeaponInstanceBySlotType(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯의 현재 무기 데이터를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	UPRWeaponDataAsset* GetWeaponDataBySlotType(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯의 현재 공개 비주얼 정보를 반환한다
	const FPRWeaponVisualInfo& GetVisualInfoBySlotType(EPRWeaponSlotType SlotType) const;

	// 인벤토리 인덱스에 있는 무기를 데이터가 지정한 슬롯에 연결한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool EquipInventoryWeaponAtIndex(int32 InventoryIndex);

	// 인벤토리 Item 식별자로 무기를 데이터가 지정한 슬롯에 연결한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool EquipWeaponItemById(const FGuid& ItemId);

	// 인벤토리 소유 무기를 데이터가 지정한 슬롯에 연결한다
	bool EquipWeapon(UPRItemInstance_Weapon* WeaponItem);

	// 지정 슬롯의 무기 연결만 해제한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool UnequipWeaponFromSlot(EPRWeaponSlotType TargetSlot);

	// 대상 슬롯 무기에 Mod를 장착하거나 교체한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	bool AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

	// 활성 무기 슬롯을 지정 슬롯으로 전환한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 현재 무장 상태를 변경한다
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Weapon")
	void SetWeaponArmedState(EPRArmedState NewArmedState);

	// 현재 활성 슬롯에 대응하는 로컬 무기 Actor를 반환한다
	UFUNCTION(BlueprintPure, Category = "ProjectR|Weapon")
	APRWeaponActor* GetActiveWeaponActor() const;

protected:
	// 서버 권위에서 슬롯 원본과 공개 상태를 갱신한다
	bool EquipWeaponInternal(UPRItemInstance_Weapon* WeaponItem);

	// 서버 권위에서 Item 식별자로 슬롯 원본과 공개 상태를 갱신한다
	bool EquipWeaponItemByIdInternal(const FGuid& ItemId);

	// 서버 권위에서 슬롯 원본 제거와 공개 상태를 갱신한다
	bool UnequipWeaponFromSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 슬롯 Mod 장착 결과를 갱신한다
	bool AttachModToSlotInternal(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

	// 서버 권위에서 현재 무기 슬롯과 공개 상태를 갱신한다
	void SetCurrentWeaponSlotInternal(EPRWeaponSlotType TargetSlot);

	// 서버 권위에서 무장 상태와 공개 상태를 갱신한다
	void SetWeaponArmedStateInternal(EPRArmedState NewArmedState);

	// 대상 슬롯의 로컬 무기 Actor를 생성 또는 갱신한다
	void RefreshWeaponActorForSlot(EPRWeaponSlotType SlotType);

	// 두 슬롯의 로컬 무기 Actor를 현재 공개 상태 기준으로 갱신한다
	void RefreshAllWeaponActors();

	// 대상 슬롯의 현재 휴대 상태에 맞춰 부착 소켓을 최신화한다
	void RefreshWeaponAttachmentForSlot(EPRWeaponSlotType SlotType);

	// 새로 복제된 활성 무기 슬롯을 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_CurrentWeaponSlot(EPRWeaponSlotType OldCurrentWeaponSlot);

	// 새로 복제된 주무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_PrimaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo);

	// 새로 복제된 보조무기 공개 비주얼 상태를 기준으로 로컬 Actor를 갱신한다
	UFUNCTION()
	void OnRep_SecondaryVisualInfo(FPRWeaponVisualInfo OldVisualInfo);

	// 새로 복제된 무장 상태를 기준으로 로컬 부착 상태를 갱신한다
	UFUNCTION()
	void OnRep_ArmedState(EPRArmedState OldArmedState);

	// 클라이언트 Item 식별자 장착 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_EquipWeaponItemById(const FGuid& ItemId);

	// 클라이언트 장비 해제 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_UnequipWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 슬롯 전환 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SetCurrentWeaponSlot(EPRWeaponSlotType TargetSlot);

	// 클라이언트 무장 상태 변경 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_SetWeaponArmedState(EPRArmedState NewArmedState);

	// 클라이언트 Mod 장착 요청을 서버 권위 경로로 전달한다
	UFUNCTION(Server, Reliable)
	void Server_AttachModToSlot(EPRWeaponSlotType TargetSlot, UPRWeaponModDataAsset* NewModData);

protected:
	// 슬롯 원본에서 공개 비주얼 정보를 구성한다
	FPRWeaponVisualInfo BuildVisualInfo(EPRWeaponSlotType SlotType, const UPRItemInstance_Weapon* WeaponItem) const;

	// 현재 슬롯이 무장 상태인지 비무장 상태인지 계산한다
	EPRArmedState GetSlotWeaponArmedState(EPRWeaponSlotType SlotType) const;

	// 슬롯과 무장 상태에 맞는 부착 소켓 이름을 반환한다
	FName GetAttachTargetSocketName(EPRWeaponSlotType SlotType, EPRArmedState SlotArmedState) const;

	// 현재 원본 슬롯 데이터 기준으로 공개 비주얼 정보를 다시 구성한다
	void RefreshVisualInfosFromCurrentState();

	// 현재 장착 상태를 기준으로 다음 활성 슬롯을 결정한다
	EPRWeaponSlotType ResolveNextWeaponSlotAfterUnequip(EPRWeaponSlotType UnequippedSlot) const;

	// 잘못된 슬롯 입력을 차단한다
	bool IsSupportedSlot(EPRWeaponSlotType SlotType) const;

	// 현재 활성 무기 데이터를 기준으로 캐릭터 애니메이션 레이어를 갱신한다
	void RefreshAnimLayer();

private:
	// 대상 슬롯 원본을 수정 가능한 참조로 반환한다
	TObjectPtr<UPRItemInstance_Weapon>& GetMutableWeaponInstanceBySlot(EPRWeaponSlotType SlotType);

	// 대상 슬롯의 현재 로컬 Actor를 반환한다
	APRWeaponActor* GetWeaponActorBySlot(EPRWeaponSlotType SlotType) const;

	// 대상 슬롯 로컬 Actor를 수정 가능한 참조로 반환한다
	TObjectPtr<APRWeaponActor>& GetMutableWeaponActorBySlot(EPRWeaponSlotType SlotType);

	// 대상 슬롯의 현재 로컬 Actor를 안전하게 정리한다
	void DestroyWeaponActorForSlot(EPRWeaponSlotType SlotType);

protected:
	// 현재 무장 상태
	UPROPERTY(ReplicatedUsing = OnRep_ArmedState, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	EPRArmedState ArmedState = EPRArmedState::Unarmed;

	// 현재 활성 무기 슬롯
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeaponSlot, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	EPRWeaponSlotType CurrentWeaponSlot = EPRWeaponSlotType::None;

	// 주무기 슬롯 공개 비주얼 정보
	UPROPERTY(ReplicatedUsing = OnRep_PrimaryVisualInfo, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualInfo PrimaryVisualInfo;

	// 보조무기 슬롯 공개 비주얼 정보
	UPROPERTY(ReplicatedUsing = OnRep_SecondaryVisualInfo, VisibleInstanceOnly, Category = "ProjectR|Weapon")
	FPRWeaponVisualInfo SecondaryVisualInfo;

	// 주무기 슬롯에 연결된 무기 Item 원본
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> PrimaryWeaponInstance;
	
	// 보조무기 슬롯에 연결된 무기 Item 원본
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPRItemInstance_Weapon> SecondaryWeaponInstance;
	
	// 04.27 김동석 추가
	// 현재 캐릭터 메시(Mesh)에 연결되어 있는 애니메이션 레이어 클래스를 추적하기 위한 캐시 변수
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> CurrentLinkedAnimLayerClass;

private:
	// 현재 머신에서만 유지하는 주무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> PrimaryWeaponActor;

	// 현재 머신에서만 유지하는 보조무기 슬롯 Actor
	UPROPERTY(Transient)
	TObjectPtr<APRWeaponActor> SecondaryWeaponActor;

	// 현재 PlayerState에 연결된 ASC 캐시
	TObjectPtr<UPRAbilitySystemComponent> CachedASC = nullptr;

	// 현재 PlayerState에 연결된 플레이어 슬롯 자원 캐시
	TObjectPtr<UPRAttributeSet_Player> CachedPlayerSet = nullptr;

	// 현재 PlayerState에 연결된 인벤토리 캐시
	TObjectPtr<UPRInventoryComponent> CachedInventory = nullptr;
};
