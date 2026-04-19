// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAttributeSet_Enemy.generated.h"

// 일반 적·보스 공통 그로기 게이지·피해 배율 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Enemy : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// GroggyGauge Max 도달 시 발행. bEntered == true면 진입, false면 해제
	UPROPERTY(BlueprintAssignable)
	FPRGroggyStateChangedSignature OnGroggyStateChanged;

	// 그로기 게이지. 0 도달 시 State.Groggy 부여
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GroggyGauge, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData GroggyGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, GroggyGauge)

	// 최대 그로기 게이지
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxGroggyGauge, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData MaxGroggyGauge;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, MaxGroggyGauge)

	// 장갑 피해 배율 (1.0 = 감산 없음)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorMultiplier, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData ArmorMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, ArmorMultiplier)

	// 약점 피해 배율
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeakpointMultiplier, Category = "ProjectR|Attributes|Enemy")
	FGameplayAttributeData WeakpointMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Enemy, WeakpointMultiplier)

protected:
	UFUNCTION() void OnRep_GroggyGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxGroggyGauge(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_ArmorMultiplier(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_WeakpointMultiplier(const FGameplayAttributeData& OldValue);
};
