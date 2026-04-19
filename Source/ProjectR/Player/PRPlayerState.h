// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerState.generated.h"

// 플레이어별 복제 데이터 소유
// Inventory/Equipment 컴포넌트는 각 시스템 구현 시 본 클래스에 부착 예정
UCLASS()
class PROJECTR_API APRPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/*~ AActor Interface ~*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 표시명 조회
	const FString& GetDisplayName() const { return DisplayName; }

	// 캐릭터 레벨 조회
	int32 GetCharacterLevel() const { return CharacterLevel; }

	// 스탯 조회 (읽기 전용)
	const FPRCharacterStats& GetStats() const { return Stats; }

	// 서버 전용. GameMode가 검증 통과한 캐릭터 페이로드를 주입. 복제는 자동
	void InitializeFromSaveData(const FPRCharacterSaveData& SaveData);

protected:
	// 표시명. 모든 클라에게 복제 (타 플레이어 HUD 표시)
	UPROPERTY(Replicated)
	FString DisplayName;

	// 캐릭터 레벨. 타 클라에도 표시될 수 있으므로 전체 복제
	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	// 캐릭터 누적 경험치. 게스트 보상 커밋 경로에서 누적
	UPROPERTY(Replicated)
	int64 Experience = 0;

	// 베이스 스탯. AttributeSet 초기화 원천
	UPROPERTY(Replicated)
	FPRCharacterStats Stats;
};
