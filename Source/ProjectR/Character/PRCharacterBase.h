// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "PRCharacterBase.generated.h"

class UPRAbilitySet;
class UAbilitySystemComponent;
class UPRAbilitySystemComponent;

// 프로젝트 전체 캐릭터 베이스. IAbilitySystemInterface 구현
// 플레이어는 PlayerState의 ASC를 위임, 적은 자기 컴포넌트를 반환 (파생 클래스에서 분기)
UCLASS(Abstract)
class PROJECTR_API APRCharacterBase : public ACharacter, public IAbilitySystemInterface
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
	
protected:
	UPROPERTY(EditAnywhere, Category = "PR Config|Character")
	FName CharacterID;
	
	UPROPERTY(EditAnywhere, Category = "PR Config|Ability")
	TObjectPtr<UPRAbilitySet> AbilitySet;
};