// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectR/Weapon/Types/PRRecoilTypes.h"
#include "GameplayTagContainer.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySet.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRWeaponDataAsset.generated.h"

class APRWeaponActor;
class USkeletalMesh;
class UAnimMontage;

// 무기 1종의 정적 장착 규칙과 기본 탄약 설정을 담는다
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponDataAsset();

public:
	// 디버그와 UI에서 구분할 무기 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FName WeaponId;

	// 무기 타입 분류
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	EPRWeaponType WeaponType = EPRWeaponType::None;

	// 무기 슬롯 타입
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	EPRWeaponSlotType SlotType = EPRWeaponSlotType::None;

	// 슬롯 초기화 시 사용할 기본 탄창 잔탄
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (ClampMin = "0"))
	int32 DefaultMagazineAmmo = 0;

	// 슬롯 초기화 시 사용할 기본 예비 탄약
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon", meta = (ClampMin = "0"))
	int32 DefaultReserveAmmo = 0;

	// 슬롯 공개 비주얼 생성에 사용할 Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TSubclassOf<APRWeaponActor> WeaponActorClass;

	// 활성 슬롯 장착 시 부여할 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon")
	TArray<FPRAbilityEntry> EquippedAbilities;

	// 활성 슬롯 장착 시 부여 직후 자동 적용할 Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "ProjectR|Weapon")
	TArray<FPREffectEntry> EquippedEffects;

	// 장착 가능한 Mod 태그 목록 (예시. Mod.Weapon.Gun 태그 있는 모드만 착용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FGameplayTagContainer SupportedModTags;
	TObjectPtr<USkeletalMesh> DisplayMesh;
	
	// 04.27 김동석 추가
	// 이 무기를 들었을 때 캐릭터에 적용할 애니메이션 레이어
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TSubclassOf<UAnimInstance> WeaponAnimLayerClass;

	// 이 무기의 카메라 반동과 크로스헤어 확산 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Recoil")
	FPRRecoilProfile RecoilProfile;
	
	// 발사 시 재생할 캐릭터 몽타주                                                   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ShootMontage;                                            
                                                                                  
	// 재장전 시 재생할 캐릭터 몽타주                                                 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;                
	
	// 발사 몽타주 재생 속도                                                           
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation") 
	float ShootMontagePlayRate = 1.0f;                                                 
                                                                                   
	// 재장전 몽타주 재생 속도                                                         
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon|Animation") 
	float ReloadMontagePlayRate = 1.0f;                                                
};
