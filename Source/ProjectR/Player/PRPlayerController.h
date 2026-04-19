// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/Game/PRGameTypes.h"
#include "PRPlayerController.generated.h"

// 플레이어 입력·UI 소유. Join 시 캐릭터 페이로드를 서버로 전송하고,
// 퇴장 시 호스트가 내려주는 보상 배치를 수령하여 GameInstance에 위임한다
UCLASS()
class PROJECTR_API APRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/*~ APlayerController Interface ~*/
	virtual void AcknowledgePossession(APawn* InPawn) override;
	virtual void BeginPlay() override;

public:
	// 로컬 클라에서 호출. GameInstance의 LocalCharacter를 서버로 제출
	// 자동 호출(BeginPlay)과 수동 호출(재제출) 모두 허용
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void SubmitLocalCharacterToServer();

	// 서버 -> 본인 클라. 캐릭터 페이로드 검증 결과. 거부 시 Detail에 사유
	UFUNCTION(Client, Reliable)
	void ClientCharacterAccepted(bool bAccepted, const FString& Detail);

	// 서버 -> 본인 클라. 퇴장 직전 누적 보상 전달. 수신 즉시 GameInstance에 커밋
	UFUNCTION(Client, Reliable)
	void ClientCommitRewards(const FPRGuestRewardBatch& Batch);

protected:
	// 클라이언트 -> 서버. 로컬 캐릭터 페이로드 제출
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSubmitCharacter(const FPRCharacterSaveData& Payload);

protected:
	// 캐릭터 페이로드를 이미 제출했는지 여부. 중복 제출 방지
	bool bCharacterSubmitted = false;
};
