// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpec.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "PRAbilitySet.generated.h"

class UPRGameplayAbility;
class UGameplayEffect;

// AbilitySet 내 어빌리티 항목
USTRUCT(BlueprintType)
struct PROJECTR_API FPRAbilityEntry
{
	GENERATED_BODY()

	// 부여할 어빌리티 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UPRGameplayAbility> AbilityClass;

	// 어빌리티 레벨. AbilitySpec.Level로 전달
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Level = 1;

	// Spec.DynamicSpecSourceTags에 주입될 태그 (InputTag 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer DynamicTags;

	// AbilityClass 유효성 검사
	bool IsValid() const;
};

// AbilitySet 내 Startup GE 항목
USTRUCT(BlueprintType)
struct PROJECTR_API FPREffectEntry
{
	GENERATED_BODY()

	// 부여 시 자동 적용할 GE 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	// GE 레벨. Level Curve 기반 값 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Level = 1.0f;

	// SpecHandle에 부여할 동적 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer DynamicTags;

	// EffectClass 유효성 검사
	bool IsValid() const;
};

// 부여 결과 핸들 묶음. Clear 시 대칭 입력
USTRUCT(BlueprintType)
struct PROJECTR_API FPRAbilitySetHandles
{
	GENERATED_BODY()

	// GiveAbility 반환 핸들 누적
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	// ApplyGameplayEffectSpecToSelf 반환 핸들 누적
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	// 두 배열 비움
	void Reset()
	{
		AbilityHandles.Reset();
		EffectHandles.Reset();
	}
};

// 어빌리티 + Startup GE 일괄 부여 단위
UCLASS(BlueprintType)
class PROJECTR_API UPRAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 부여할 어빌리티 목록. 순서대로 GiveAbility
	UPROPERTY(EditAnywhere, Category = "AbilitySet")
	TArray<FPRAbilityEntry> Abilities;

	// 부여 직후 자동 적용할 Startup GE 목록
	UPROPERTY(EditAnywhere, Category = "AbilitySet")
	TArray<FPREffectEntry> Effects;
	
public:
	UPRAbilitySet();
};