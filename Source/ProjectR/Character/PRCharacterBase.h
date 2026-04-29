// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/Combat/PRCombatInterface.h"
#include "PRCharacterBase.generated.h"

struct FGameplayTag;
class UPRAbilitySet;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;

// 프로젝트 전체 캐릭터 베이스. IAbilitySystemInterface와 IPRCombatInterface를 구현한다.
// 플레이어는 PlayerState의 ASC를 위임, 적은 자기 컴포넌트를 반환 (파생 클래스에서 분기)
UCLASS(Abstract)
class PROJECTR_API APRCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPRCombatInterface
{
	GENERATED_BODY()

public:
	APRCharacterBase();

	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaTime) override;

	/*~ ACharacter Interface ~*/
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	// 프로젝트 ASC 타입으로 반환
	virtual UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const;

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	/*~ APRCharacterBase Interface ~*/
	void BindTagChangeEvent();
	// ASC의 OwnedTags 스택 변경 이벤트 핸들러, TagExists가 true 면 태그 추가, false면 태그 제거
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool bTagExists);
	
protected:
	UPROPERTY(EditAnywhere, Category = "PR Config|Character")
	FName CharacterID;
	
	UPROPERTY(EditAnywhere, Category = "PR Config|Ability")
	TObjectPtr<UPRAbilitySet> AbilitySet;
};