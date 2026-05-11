// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Weapon/Types/PRWeaponTypes.h"
#include "PRAbilitySystemRegistry.generated.h"

class UGameplayEffect;
// 프로젝트 단일 인스턴스 (DA_AbilitySystemRegistry). 역할별 DT와 Attribute 매핑 보유
UCLASS(BlueprintType)
class PROJECTR_API UPRAbilitySystemRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*~ UPrimaryDataAsset Interface ~*/
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	// PropertyToAttribute 조회. 누락 시 비유효 Attribute 반환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	FGameplayAttribute FindAttribute(FName PropertyName) const;

	// Role별 StatTable 소프트 레퍼런스를 LoadSynchronous로 해석
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	UDataTable* GetStatTableSynchronous(EPRCharacterRole Role) const;

	// 탄약 타입에 대응하는 무기 장착 GE 반환 (슬롯별 어트리뷰트 set 대상이 다르므로 분리)
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	TSubclassOf<UGameplayEffect> GetEquipAmmoGE(EPRAmmoType AmmoType) const;

public:
	// 역할별 스탯 DataTable 약참조
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<EPRCharacterRole, TSoftObjectPtr<UDataTable>> StatTables;
	
	// Row의 FFloatProperty 이름 -> 대응 Attribute 매핑 (PrimaryAttributes)
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<FName, FGameplayAttribute> PropertyToAttribute;
	
	// 초기화 GE
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSubclassOf<UGameplayEffect>  InitializeGE;
	
	// ==== 데미지 적용 GE ====
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSubclassOf<UGameplayEffect>  DamageGE_FromEnemy;
	
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSubclassOf<UGameplayEffect>  DamageGE_FromWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSubclassOf<UGameplayEffect>  DamageGE_FromMod;

	// ==== 무기 장착 GE (슬롯별, AmmoScale·ReserveAmmoRatio·MagazineAmmo set용) ====
	// 주무기 슬롯 어트리뷰트를 set하는 Instant GE
	UPROPERTY(EditAnywhere, Category = "Registry|Ammo")
	TSubclassOf<UGameplayEffect> EquipAmmoGE_Primary;

	// 보조무기 슬롯 어트리뷰트를 set하는 Instant GE
	UPROPERTY(EditAnywhere, Category = "Registry|Ammo")
	TSubclassOf<UGameplayEffect> EquipAmmoGE_Secondary;

	// ==== 쿨다운 GE ====
	// 사격 어빌리티 공통 쿨다운 GE. SetByCaller.Cooldown으로 Duration 주입. Cooldown.Ability.Fire.Primary를 Granted Tag로 부여
	UPROPERTY(EditAnywhere, Category = "Registry|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownGE_Fire;
};
