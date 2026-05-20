// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "ProjectR/Weapon/Types/PRRecoilTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Inventory/Data/PRItemDataAsset.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponDataAsset.generated.h"

class APRWeaponActor;
class USkeletalMesh;
class UAnimMontage;
class UDataTable;
class UGameplayEffect;
class UUserWidget;

// 무기 1종의 정적 장착 규칙과 기본 탄약 설정을 담는다
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponDataAsset : public UPRItemDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponDataAsset();

	// 탄약 타입 반환 (무기 슬롯 타입에 따름)
	UFUNCTION(BlueprintPure)
	EPRAmmoType GetAmmoType() const {return SlotType == EPRWeaponSlotType::Primary ? EPRAmmoType::Primary : EPRAmmoType::Secondary;}

public:
	// 무기 타입 분류
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	EPRWeaponType WeaponType = EPRWeaponType::None;

	// 무기 슬롯 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;
	
	// 무기 Armed 부착 소켓 이름 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	EPRWeaponArmedSocketNames ArmedSocketName;
	
	// 무기 Unarmed 부착 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	EPRWeaponStowedSocketNames StowedSocketName;

	// 무기 전용 강화 단계 비용 데이터 테이블
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Upgrade")
	TObjectPtr<UDataTable> WeaponUpgradeTable = nullptr;
	
	// ========= 무기 장착 GE 참조 값 ===============
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Stat", meta = (ClampMin = "0.0"))
	float BaseDamage = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Stat", meta = (ClampMin = "0.0"))
	float ArmorPenetration = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Stat", meta = (ClampMin = "0.0"))
	float GroggyDamageMultiplier = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Stat", meta = (ClampMin = "0.0"))
	float WeakpointMultiplier = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Ammo", meta = (ClampMin = "0.0"))
	float MaxMagazineAmmo = 30.f;
	
	// 보유 한도 비율. MaxMagazineAmmo 대비 예비탄 한도 배수
	// 5.0 = 탄창 5개분 예비탄 보유 가능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Ammo", meta = (ClampMin = "0.0"))
	float ReserveAmmoMultiplier = 5.0f;

	// 슬롯 공개 비주얼 생성에 사용할 Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	TSubclassOf<APRWeaponActor> WeaponActorClass;

	// 활성 슬롯 장착 시 부여할 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "ProjectR|01_Weapon")
	TArray<FPRAbilityEntry> EquippedAbilities;

	// 활성 슬롯 장착 시 부여 직후 자동 적용할 Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "ProjectR|01_Weapon")
	TArray<FPREffectEntry> EquippedEffects;

	// 장착 가능한 Mod 태그 목록 (예시. Mod.Weapon.Gun 태그 있는 모드만 착용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	FGameplayTagContainer SupportedModTags;
	
	// 04.27 김동석 추가
	// 이 무기를 들었을 때 캐릭터에 적용할 애니메이션 레이어
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	// 이 무기의 카메라 반동과 크로스헤어 확산 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Recoil")
	FPRRecoilProfile RecoilProfile;
	
	// 발사 시 재생할 캐릭터 몽타주                                                   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Animation")
	TObjectPtr<UAnimMontage> ShootMontage;                                            
                                                                                  
	// 재장전 시 재생할 캐릭터 몽타주                                                 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;                
	
	// 발사 몽타주 재생 속도                                                           
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Animation") 
	float ShootMontagePlayRate = 1.0f;                                                 
                                                                                   
	// 재장전 몽타주 재생 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Animation")
	float ReloadMontagePlayRate = 1.0f;

	// 발사 간격(초). RPM = 60 / FireInterval. 0.1 = 600 RPM
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|Fire", meta = (ClampMin = "0.01"))
	float FireInterval = 0.1f;

	// 무기 장착 중 HUD에 준비할 스코프 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|01_Weapon|UI")
	TSubclassOf<UUserWidget> ScopeWidgetClass;
};
