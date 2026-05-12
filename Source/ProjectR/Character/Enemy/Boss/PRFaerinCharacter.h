// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/Character/Enemy/PRBossBaseCharacter.h"
#include "PRFaerinCharacter.generated.h"

// Faerin 보스 본체 클래스다.
// 패턴 분기와 포털/검 생성 로직은 넣지 않고, 보스 공통 베이스에 Faerin 기본 데이터만 얹는다.
UCLASS(Blueprintable)
class PROJECTR_API APRFaerinCharacter : public APRBossBaseCharacter
{
	GENERATED_BODY()

public:
	APRFaerinCharacter();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

private:
	// 테스트용 보스 HUD 바인딩을 시도한다.
	void TryBindBossHealthBar();

	// 테스트용 보스 HUD 바인딩 재시도 타이머를 처리한다.
	void HandleBossHealthBarBindRetry();

private:
	// 테스트용 보스 HUD 바인딩 재시도 간격
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|Boss")
	float BossHealthBarBindRetryInterval = 0.1f;

	// 테스트용 보스 HUD 바인딩 최대 재시도 횟수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|HUD|Boss")
	int32 MaxBossHealthBarBindRetryCount = 20;

	FTimerHandle BossHealthBarBindRetryTimerHandle;
	int32 CurrentBossHealthBarBindRetryCount = 0;
	bool bBossHealthBarBound = false;
};
