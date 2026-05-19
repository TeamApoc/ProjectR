// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "ProjectR/AbilitySystem/PRAbilityTypes.h"
#include "ProjectR/Character/PRCharacterBase.h"
#include "PRGameplayAbility.generated.h"

class UPRWeaponDataAsset;
class UPRWeaponManagerComponent;

UENUM(BlueprintType)
enum class EPRPlayerAbilityType : uint8
{
	None,
	Weapon,
	Mod,
};

// 프로젝트 공통 어빌리티 베이스. 모든 어빌리티는 이 클래스를 상속
UCLASS(Abstract)
class PROJECTR_API UPRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                 const FGameplayAbilityActorInfo* ActorInfo,
	                                 const FGameplayTagContainer* SourceTags,
	                                 const FGameplayTagContainer* TargetTags,
	                                 FGameplayTagContainer* OptionalRelevantTags) const override;


	// CDO가 아닌 어빌리티의 실제 인스턴스 반환
	template<typename T>
	T* GetAbilityInstance(const FGameplayAbilitySpecHandle Handle,  const FGameplayAbilityActorInfo* ActorInfo) const
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			if (FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(Handle))
			{
				return Cast<T>(AbilitySpec->GetPrimaryInstance());
			}
		}
		return nullptr;
	}
	
	
	// ActivationPolicy 조회. ASC OnGiveAbility 및 입력 라우터에서 사용
	EPRAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	// InputTag 조회
	const FGameplayTag& GetInputTag() const { return InputTag; }
	
	// Instance된 어빌리티의 SourceObject(ItemInstance)로부터 WeaponData를 반환
	UPRWeaponDataAsset* GetCurrentWeaponData() const;
	
	
	// PlayerCharacter의 WeaponManager 반환
	UPRWeaponManagerComponent* GetWeaponManager(const FGameplayAbilityActorInfo* ActorInfo) const;
	
	// 현재 활성 무기 데이터를 반환                           
	UPRWeaponDataAsset* GetActiveWeaponData(const FGameplayAbilityActorInfo* ActorInfo) const;              
    
	// PlayerAbilityType에 따른 EffectSpec 만들어 반환
	virtual FGameplayEffectSpecHandle MakePlayerEffectSpec(const FHitResult* HitResult = nullptr, float Damage = 0.f, float GroggyDamage = 0.0f);
	
	// 무기 데미지 EffectSpec 반환. 기본은 Registry의 DamageGE_FromWeapon 사용. 파생 클래스에서 다른 GE로 교체 가능
	virtual FGameplayEffectSpecHandle MakeWeaponEffectSpec(const FHitResult* HitResult = nullptr) const;
	
	// 모드 스킬의 EffectSpec 반환, Damage와 GroggyDamage를 SetByCaller로 전달.
	// 기본 구현은 Registry의 DamageGE_FromMod 사용. 파생 클래스에서 다른 GE로 교체 가능
	virtual FGameplayEffectSpecHandle MakeModEffectSpec(float Damage, float GroggyDamage = 0.0f, const FHitResult* HitResult = nullptr) const;


	UFUNCTION(BlueprintCallable)
	virtual void ApplySourceModCost() const;
	
protected:
	// AvatarActor를 지정한 캐릭터 타입으로 캐스팅하여 반환. CharacterType은 APRCharacterBase 파생만 허용
	template <typename CharacterType>
	CharacterType* GetPRCharacter() const
	{
		static_assert(TIsDerivedFrom<CharacterType, APRCharacterBase>::IsDerived,
			"CharacterType must be derived from APRCharacterBase.");
		return Cast<CharacterType>(GetAvatarActorFromActorInfo());
	}

public:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ability")
	EPRPlayerAbilityType PlayerAbilityType = EPRPlayerAbilityType::None;
	
protected:
	// 본 어빌리티의 활성화 정책
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ability")
	EPRAbilityActivationPolicy ActivationPolicy = EPRAbilityActivationPolicy::OnInputTriggered;

	// 플레이어 입력 바인딩 태그. ServerInvoked/OnGiven은 비워둠
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Ability", meta = (Categories = "Input.Ability"))
	FGameplayTag InputTag;
};
