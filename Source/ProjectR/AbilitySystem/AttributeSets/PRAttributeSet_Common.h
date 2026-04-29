// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "PRAttributeSet_Common.generated.h"

// 모든 캐릭터 공통 체력·이동 속성 세트
UCLASS()
class PROJECTR_API UPRAttributeSet_Common : public UAttributeSet
{
	GENERATED_BODY()

public:
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/*~ UObject Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 사망이벤트. Instigator는 최종 타격 소스 액터
	UPROPERTY(BlueprintAssignable)
	FPRDeathSignature OnDeath;

	// 현재 체력. 0 도달 시 State.Dead 부여 + OnDeath 발행
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "ProjectR|Attributes|Common")
	FGameplayAttributeData Health;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Common, Health)

	// 최대 체력. Health 클램프 상한
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "ProjectR|Attributes|Common")
	FGameplayAttributeData MaxHealth;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Common, MaxHealth)

	// 이동 속도 배율 (1.0 기준)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeedMultiplier, Category = "ProjectR|Attributes|Common")
	FGameplayAttributeData MovementSpeedMultiplier;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Common, MovementSpeedMultiplier)

	// 방어력
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "ProjectR|Attributes|Common")
	FGameplayAttributeData Armor;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Common, Armor)

	// 받는 데미지. 메타 어트리뷰트로 체력 차감 처리 후 즉시 0으로 초기화됨
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|Attributes|Common", meta = (HideFromModifiers))
	FGameplayAttributeData IncomingDamage;
	PR_ATTRIBUTE_ACCESSORS(UPRAttributeSet_Common, IncomingDamage)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MovementSpeedMultiplier(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);
};
