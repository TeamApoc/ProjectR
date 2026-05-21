// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRPlayerState.generated.h"

struct FPRInventoryChangeEventData;
enum class EPRInventoryChangeReason : uint8;
class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UPRAttributeSet_Player;
class UPRAttributeSet_Weapon;
class UPRInventoryComponent;
class UPREquipmentManagerComponent;
class UPRQuickSlotComponent;
class UPRCurrencyComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMouseSensitivityChanged, float, NewSensitivity);

// 플레이어별 복제 데이터 소유. Player ASC + AttributeSet의 소유자
// Inventory/Equipment 컴포넌트는 각 시스템 구현 시 본 클래스에 부착 예정
UCLASS()
class PROJECTR_API APRPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APRPlayerState();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	
	/*~ APlayerState Interface ~*/
	virtual void CopyProperties(APlayerState* PlayerState) override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	
public:
	// 프로젝트 ASC 타입으로 반환
	UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const { return AbilitySystemComponent; }

	// 공통 AttributeSet 조회
	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }

	// 플레이어 AttributeSet 조회
	UPRAttributeSet_Player* GetPlayerSet() const { return PlayerSet; }

	// 무기 자원 AttributeSet 조회
	UPRAttributeSet_Weapon* GetWeaponSet() const { return WeaponSet; }

	// 플레이어 인벤토리 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Inventory")
	UPRInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// 플레이어 장비 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Equipment")
	UPREquipmentManagerComponent* GetEquipmentManagerComponent() const { return EquipmentManagerComponent; }

	// 플레이어 퀵슬롯 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|QuickSlot")
	UPRQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

	// 플레이어 재화 컴포넌트를 반환
	UFUNCTION(BlueprintPure, Category = "ProjectR|Currency")
	UPRCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }

	// 지정 무기 슬롯의 캐시된 탄창과 예비탄 비율을 반환한다
	void GetCachedAmmoRatios(EPRWeaponSlotType SlotType, float& OutMagazineRatio, float& OutReserveRatio) const;

	// 지정 무기 슬롯의 탄창과 예비탄 비율을 캐시한다
	void SetCachedAmmoRatios(EPRWeaponSlotType SlotType, float MagazineRatio, float ReserveRatio);

	// 표시명 조회
	const FString& GetDisplayName() const { return DisplayName; }

	// 캐릭터 레벨 조회
	int32 GetCharacterLevel() const { return CharacterLevel; }

	// 캐릭터 업그레이드 정보 조회
	FPRCharacterStatUpgradeInfo GetStatUpgradeInfo() const { return StatUpgradeInfo; }

	// 현재 플레이어가 전멸 판정에 포함되는 전투 참여자인지 확인한다.
	bool IsCombatParticipant() const;

	// 현재 플레이어가 다운 상태인지 확인한다.
	bool IsDown() const;

	// 현재 플레이어가 최종 사망 상태인지 확인한다.
	bool IsDead() const;

	// 현재 플레이어가 다운 또는 사망으로 전투에서 이탈했는지 확인한다.
	bool IsOutOfFight() const;

	// 자신을 제외한 전투 참여자 중 아직 전투 가능한 플레이어가 있는지 확인한다.
	bool HasFightCapableAllyExceptSelf() const;

	// 현재 플레이어 Pawn에게 생존 상태 전환 이벤트를 보낸다.
	void SendSurvivalGameplayEvent(const FGameplayTag& EventTag) const;

	// 리스폰 전 생존 상태 태그와 입력 캐시를 초기화한다
	void ResetSurvivalStateForRespawn();

	// 현재 캐릭터 Pawn 기준 AbilitySet을 재부여한다
	void GrantCharacterAbilitySet(const UPRAbilitySet* InAbilitySet, UObject* InSourceObject = nullptr);
	
	// 기본 정보 적용 (맵 전환시 유지하기 위해)
	void InitializePrimaryInfoFromSaveData(const FPRCharacterSaveData& InSaveData);

	// 서버 전용. AbilitySystem, Inventory등의 상태를 SaveData에서 복원. (Character의 PossessedBy에서 AbilitySystem 초기화 후 호출) 
	void ApplySaveData(const FPRCharacterSaveData& InSaveData);
	
	// TODO: 플레이어 각종 상태 값 (인벤토리 포함) 기록하여 반환
	FPRCharacterSaveData MakeSaveData() const;
	
	// 마우스 감도
	float GetCameraSensitivity() const { return CameraSensitivity; }
	void SetCameraSensitivity(float Sensitivity);
	FOnMouseSensitivityChanged OnMouseSensitivityChanged;
	
	
protected:
	void BindAutoRegisterQuickSlotEvent();
	
	UFUNCTION()
	void OnInventoryChanged(UPRInventoryComponent* InInventory, const FPRInventoryChangeEventData& EventData);
	
protected:
	// ===== Configs ======
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Player")
	TArray<FPRItemSaveEntry> StartUpItems;
	
	// ===== Information =====
	// 표시명. 모든 클라에게 복제 (타 플레이어 HUD 표시)
	UPROPERTY(Replicated)
	FString DisplayName;

	// 캐릭터 레벨. 타 클라에도 표시될 수 있으므로 전체 복제
	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	// 캐릭터 누적 경험치. 게스트 보상 커밋 경로에서 누적
	UPROPERTY(Replicated)
	int64 Experience = 0;

	// 스탯 업그레이드 정보
	UPROPERTY(Replicated)
	FPRCharacterStatUpgradeInfo StatUpgradeInfo;
	
	// ===== Components =====
	// 플레이어 ASC. PlayerState에 부착
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	// 공통 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	// 플레이어 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Player> PlayerSet;

	// 무기 속성
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Weapon> WeaponSet;

	// 플레이어가 소유한 Item 인스턴스 정본 컨테이너
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Inventory")
	TObjectPtr<UPRInventoryComponent> InventoryComponent;

	// 플레이어 장비 상태 확장용 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Equipment")
	TObjectPtr<UPREquipmentManagerComponent> EquipmentManagerComponent;

	// 플레이어 소비 아이템 퀵슬롯 상태 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|QuickSlot")
	TObjectPtr<UPRQuickSlotComponent> QuickSlotComponent;

	// 플레이어가 보유한 고철 재화 컨테이너
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Currency")
	TObjectPtr<UPRCurrencyComponent> CurrencyComponent;

	// 주무기 슬롯 탄창 보존 비율
	float CachedPrimaryMagazineAmmoRatio = 1.0f;

	// 주무기 슬롯 예비탄 보존 비율
	float CachedPrimaryReserveAmmoRatio = 1.0f;

	// 보조무기 슬롯 탄창 보존 비율
	float CachedSecondaryMagazineAmmoRatio = 1.0f;

	// 보조무기 슬롯 예비탄 보존 비율
	float CachedSecondaryReserveAmmoRatio = 1.0f;

	// 현재 캐릭터 Pawn에서 부여한 AbilitySet 핸들
	FPRAbilitySetHandles CharacterAbilitySetHandles;
	
	/** 카메라 감도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Camera")
	float CameraSensitivity = 0.5f;
private:
	UPROPERTY()
	FPRCharacterSaveData CurrentSaveData;
};
