// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PRGameTypes.h"
#include "PRPlayGameMode.generated.h"

class APRPlayerController;

// 인게임 GameMode. 호스트 프로세스에만 존재
// 로그인 승인, 게스트 캐릭터 페이로드 검증, 월드 상태 주입, 이탈 시 보상 커밋을 담당
UCLASS()
class PROJECTR_API APRPlayGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APRPlayGameMode();

	/*~ AGameModeBase Interface ~*/
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

public:
	// 월드 이벤트 반영. 체크포인트 트리거·보스 처치 등에서 호출
	void ReportCheckpointActivated(FName CheckpointId);
	void ReportBossDefeated(FName BossId);

	// 게스트가 ServerSubmitCharacter로 보낸 페이로드를 검증하고 PlayerState에 주입
	// 반환값이 false면 호출측(PlayerController)이 ClientCharacterAccepted(false)로 거부 통지
	bool AcceptGuestCharacter(APRPlayerController* From, const FPRCharacterSaveData& Payload);

	// 보상 발생 이벤트에서 호출. 해당 오너 클라에게 즉시 Grant를 푸시
	// 같은 프레임 내 중복 지급은 호출자가 합산하여 1회로 보낸다
	void GrantRewardTo(APRPlayerController* Target, const FPRRewardGrant& Grant);

protected:
	// 페이로드 검증. 실패 시 OutReason에 사유 기록
	bool ValidateCharacterPayload(const FPRCharacterSaveData& Payload, FString& OutReason) const;

protected:
	// 호스트 시작 시 로드된 월드 세이브. InitGame에서 주입되어 GameState에 전달된다
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|World")
	FPRWorldSaveData HostWorldSave;

	// 페이로드 검증 상한 기본값. 추후 UPRGameplayConfig DataAsset으로 외부화
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Validation")
	int32 MaxCharacterLevel = 100;

	// 표시명 최대 길이
	UPROPERTY(EditDefaultsOnly, Category = "ProjectR|Validation")
	int32 MaxDisplayNameLength = 24;
};
