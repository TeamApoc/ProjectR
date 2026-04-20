// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/NetworkDelegates.h"
#include "PRGameTypes.h"
#include "PRSessionSubsystem.generated.h"

// 세션 상태 변화 통지. UI가 Bind하여 로비/로딩 화면을 갱신
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChangedSignature, EPRSessionState, NewState);

// 세션 실패 통지. Reason은 사용자 표시용, Detail은 로그·디버그용
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionFailedSignature, EPRSessionFailReason, Reason, FString, Detail);

// 세션 Host/Join의 플랫폼별 구현을 격리. 상위는 동일 인터페이스로 호출
// 현재 구현: IP 직접 접속. 추후 OSS(Steam/EOS) 연동 시 본 클래스의 내부 구현만 교체한다
UCLASS()
class PROJECTR_API UPRSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/*~ UGameInstanceSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	// 호스트 개시. 성공 시 리슨 서버로 OpenLevel, 실패 시 OnSessionFailed 발행
	UFUNCTION(BlueprintCallable)
	void StartHost(const FPRHostSessionParams& Params);

	// 참가 개시. 주소 파싱 실패/접속 실패 시 OnSessionFailed 발행
	UFUNCTION(BlueprintCallable)
	void StartJoin(const FPRJoinSessionParams& Params);

	// 현재 세션 종료 처리. 호스트/게스트 공통 진입점
	void EndSession();

	// 현재 세션 상태 조회
	EPRSessionState GetState() const { return CurrentState; }

protected:
	// 상태 전이 + 델리게이트 발행
	void SetState(EPRSessionState NewState);

	// 네트워크 레이어 실패 핸들러. ClientTravel 실패·연결 끊김 포착
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// 주소 포맷 검증. IPv4(:Port)? 만 허용
	bool ValidateAddress(const FString& Address) const;

public:
	// 세션 상태 변화 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnSessionStateChangedSignature OnSessionStateChanged;

	// 세션 실패 이벤트
	UPROPERTY(BlueprintAssignable)
	FOnSessionFailedSignature OnSessionFailed;

protected:
	// 현재 세션 진행 단계
	EPRSessionState CurrentState = EPRSessionState::None;

	// 호스트 개시 시 보관된 파라미터. 맵 전이 이후 로직에서 참조
	FPRHostSessionParams PendingHostParams;

	// 메뉴 복귀 시 사용할 맵 이름. 프로젝트 설정에서 주입
	FName MenuMapName = TEXT("MainMenu");
};
