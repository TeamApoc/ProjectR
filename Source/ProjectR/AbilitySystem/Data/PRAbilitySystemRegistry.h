// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayEffectTypes.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
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

	// 초기화 GE를 동기 로딩하여 반환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Ability")
	TSubclassOf<UGameplayEffect> GetInitializeGESynchronous() const;
	
public:
	// 역할별 스탯 DataTable 약참조
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<EPRCharacterRole, TSoftObjectPtr<UDataTable>> StatTables;
	
	// Row의 FFloatProperty 이름 -> 대응 Attribute 매핑 (PrimaryAttributes)
	UPROPERTY(EditAnywhere, Category = "Registry")
	TMap<FName, FGameplayAttribute> PropertyToAttribute;
	
	// 초기화 GE
	UPROPERTY(EditAnywhere, Category = "Registry")
	TSoftClassPtr<UGameplayEffect>  InitializeGE;
};
