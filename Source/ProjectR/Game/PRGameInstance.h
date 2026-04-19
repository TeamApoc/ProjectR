// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PRGameTypes.h"
#include "PRGameInstance.generated.h"

class UPRSessionSubsystem;

// 프로세스 전체 수명의 최상위 컨텍스트. 로컬 세이브와 세션 진입점을 보유한다
// 실제 세션 Host/Join 동작은 UPRSessionSubsystem에 위임한다
UCLASS()
class PROJECTR_API UPRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/*~ UGameInstance Interface ~*/
	virtual void Init() override;
	virtual void Shutdown() override;

public:
	// 세션 Host 진입점. 실패 사유는 SessionSubsystem의 OnSessionFailed로 통지
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void HostSession(const FPRHostSessionParams& Params);

	// 세션 Join 진입점. 주소 포맷은 IP 방식과 OSS 결과 모두 동일하게 취급
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void JoinSession(const FPRJoinSessionParams& Params);

	// 현재 세션에서 퇴장. 클라는 메뉴 맵으로 복귀, 호스트는 서버 종료 후 복귀
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Session")
	void LeaveSession();

	// 로컬 캐릭터 세이브 로드. 메뉴에서 "이어하기" 선택 시 호출
	// 현재 단계에서는 스켈레톤 구현. 세이브 시스템 연동 시 본 메서드에서 SaveGame 로드 분기 추가
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool LoadLocalCharacter(FName SlotName);

	// 로컬 캐릭터 세이브 저장
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	bool SaveLocalCharacter(FName SlotName);

	// 보상 이벤트 발생 시 서버가 푸시한 Grant를 로컬 캐릭터에 즉시 반영
	// PlayerController.ClientGrantReward 수신 경로의 종착점
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Save")
	void ApplyRewardGrant(const FPRRewardGrant& Grant);

	// 현재 로컬 캐릭터 데이터 조회. Join 시 PlayerController가 서버로 제출할 때 사용
	const FPRCharacterSaveData& GetLocalCharacter() const { return LocalCharacter; }

	// 로컬 캐릭터 직접 갱신. 신규 캐릭터 생성 UI 등에서 호출
	void SetLocalCharacter(const FPRCharacterSaveData& NewData) { LocalCharacter = NewData; }

protected:
	// 현재 플레이어가 들고 다니는 캐릭터 스펙. Join 시 이 데이터가 호스트로 전송된다
	UPROPERTY(VisibleInstanceOnly, Category = "ProjectR|Save")
	FPRCharacterSaveData LocalCharacter;
};
