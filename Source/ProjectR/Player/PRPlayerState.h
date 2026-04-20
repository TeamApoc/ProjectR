// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerState.generated.h"

class UPRAbilitySystemComponent;
class UPRAttributeSet_Common;
class UPRAttributeSet_Player;

// 플레이어별 복제 데이터 소유. Player ASC + AttributeSet의 소유자
// Inventory/Equipment 컴포넌트는 각 시스템 구현 시 본 클래스에 부착 예정
UCLASS()
class PROJECTR_API APRPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APRPlayerState();

	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/*~ IAbilitySystemInterface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	// 프로젝트 ASC 타입으로 반환
	UPRAbilitySystemComponent* GetPRAbilitySystemComponent() const { return AbilitySystemComponent; }

	// 공통 AttributeSet 조회
	const UPRAttributeSet_Common* GetCommonSet() const { return CommonSet; }

	// 플레이어 AttributeSet 조회
	const UPRAttributeSet_Player* GetPlayerSet() const { return PlayerSet; }

	// 표시명 조회
	const FString& GetDisplayName() const { return DisplayName; }

	// 캐릭터 레벨 조회
	int32 GetCharacterLevel() const { return CharacterLevel; }

	// 캐릭터 업그레이드 정보 조회
	FPRCharacterStatUpgradeInfo GetStatUpgradeInfo() const { return StatUpgradeInfo; }
	
	// 서버 전용. GameMode가 검증 통과한 캐릭터 페이로드를 주입. 복제는 자동
	void InitializeFromSaveData(const FPRCharacterSaveData& SaveData);

protected:
	// 플레이어 ASC. PlayerState에 부착
	UPROPERTY(VisibleAnywhere, Category = "ProjectR|Ability")
	TObjectPtr<UPRAbilitySystemComponent> AbilitySystemComponent;

	// 공통 속성 (Health/MaxHealth/MoveSpeedMul)
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Common> CommonSet;

	// 플레이어 속성 (Stamina/Mod/Ammo)
	UPROPERTY()
	TObjectPtr<UPRAttributeSet_Player> PlayerSet;

	// 표시명. 모든 클라에게 복제 (타 플레이어 HUD 표시)
	UPROPERTY(Replicated)
	FString DisplayName;

	// 캐릭터 레벨. 타 클라에도 표시될 수 있으므로 전체 복제
	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	// 캐릭터 누적 경험치. 게스트 보상 커밋 경로에서 누적
	UPROPERTY(Replicated)
	int64 Experience = 0;

	// 스탯 업그레이드 정보 
	UPROPERTY(Replicated)
	FPRCharacterStatUpgradeInfo StatUpgradeInfo;
};
