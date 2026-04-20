// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PRGameTypes.generated.h"

/*~ 세이브 포맷 버전 ~*/

// 세이브 파일 포맷 버전. Join 시 호스트-게스트 간 호환성 체크에 사용
// 불일치 시 접속 거부. 상향은 마이그레이션 구현 후에만 허용
UENUM(BlueprintType)
enum class EPRSaveVersion : uint8
{
	// 미지정 (기본값)
	None = 0,
	// 최초 포맷
	V1   = 1,
};

/*~ 세션 상태 ~*/

// 세션 진행 단계. UI 로딩/에러 표시 분기용
UENUM(BlueprintType)
enum class EPRSessionState : uint8
{
	// 메뉴 상태(세션 없음)
	None,
	// 호스트 개시 진행 중(맵 로딩)
	Hosting,
	// 호스트 완료(인게임)
	Hosted,
	// 참가 진행 중(접속 시도)
	Joining,
	// 참가 완료(인게임)
	Joined,
	// 종료 처리 중
	Leaving,
};

// 세션 실패 사유 분류. UI 로컬라이즈 키 매핑에 사용
UENUM(BlueprintType)
enum class EPRSessionFailReason : uint8
{
	// 알 수 없음
	Unknown,
	// 주소 형식 오류
	InvalidAddress,
	// 네트워크 레이어 실패(연결 거부, 타임아웃 등)
	NetworkFailure,
	// 맵 로딩 실패
	MapLoadFailed,
	// 세이브 포맷 버전 불일치
	VersionMismatch,
	// 캐릭터 페이로드 검증 실패
	CharacterRejected,
};

/*~ 월드 오브젝트 상태 ~*/

// 문·게이트 등 환경 오브젝트의 상태 열거. WorldSave에 매핑되어 세션 동안 유지
UENUM(BlueprintType)
enum class EPRWorldObjectState : uint8
{
	// 초기 상태(잠김/닫힘)
	Default,
	// 열림/활성화됨
	Opened,
	// 제거됨(일회성 오브젝트)
	Consumed,
};

/*~ 인벤토리 / 장비 (추후 구현) ~*/

// [확장 및 이전 예정] 인벤토리 단일 슬롯 엔트리. Inventory 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPRItemEntry
{
	GENERATED_BODY()
};

// [확장 및 이전 예정] 세이브/네트워크 전송용 인벤토리 직렬화 DTO. Inventory 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPRInventorySnapshot
{
	GENERATED_BODY()
};

// [확장 및 이전 예정] 장착 중인 무기/장비 슬롯 매핑. Equipment 시스템 구현 시 필드 확정
USTRUCT(BlueprintType)
struct FPREquipmentState
{
	GENERATED_BODY()
};

/*~ 캐릭터 스펙 ~*/

// 캐릭터 스탯 업그레이드 정보.
USTRUCT(BlueprintType)
struct FPRCharacterStatUpgradeInfo
{
	GENERATED_BODY()
};

// 게스트가 Join 시 호스트에 전송하는 캐릭터 스펙 번들
// 호스트는 수신 즉시 Validate -> PlayerState에 주입 후 복제
USTRUCT(BlueprintType)
struct FPRCharacterSaveData
{
	GENERATED_BODY()

	// 세이브 포맷 버전. 불일치 시 호스트가 접속 거부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRSaveVersion Version = EPRSaveVersion::V1;

	// 플레이어 표시명. 텍스트 최대 길이 제한(서버 검증)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	// 캐릭터 레벨. 서버는 현재 허용된 상한과 비교하여 초과 시 클램프
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	// 누적 경험치. Level과의 정합성도 함께 검증
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Experience = 0;

	// 스탯 배분 결과. 각 필드는 Level 기반 상한 검증 대상
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPRCharacterStatUpgradeInfo Stats;
};

/*~ 월드 세이브 ~*/

// 호스트 세이브에서 로드되어 GameState에 복제되는 월드 진행 상태
USTRUCT(BlueprintType)
struct FPRWorldSaveData
{
	GENERATED_BODY()

	// 세이브 포맷 버전
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPRSaveVersion Version = EPRSaveVersion::V1;

	// 마지막으로 활성화된 체크포인트 ID. 리스폰 지점 결정에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LastCheckpointId;

	// 활성화된 체크포인트 집합. UI 목록·워프에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> UnlockedCheckpoints;

	// 처치 완료된 보스 ID 집합. 재도전 시 상태 초기화 분기
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> DefeatedBosses;

	// 열린 문·게이트 등 환경 오브젝트 상태. 키는 오브젝트 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, EPRWorldObjectState> WorldObjectStates;

	// 스토리 진행 플래그. 퀘스트·컷신 트리거 제어용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer StoryFlags;
};

/*~ 세션 연결 파라미터 ~*/

// Host 개시 시 GameInstance로 전달되는 파라미터
USTRUCT(BlueprintType)
struct FPRHostSessionParams
{
	GENERATED_BODY()

	// 호스팅할 맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MapName;

	// 허용 최대 인원. GameMode의 MaxPlayers로 반영
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers = 4;

	// 비공개 여부. 현재는 표시용. OSS 단계에서 세션 검색 필터에 사용 예정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPrivate = false;
};

// Join 시도 시 전달되는 파라미터
USTRUCT(BlueprintType)
struct FPRJoinSessionParams
{
	GENERATED_BODY()

	// 접속 주소. IP 방식: "192.168.0.10:7777". OSS 단계에서는 세션 Resolve 결과
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Address;
};

/*~ 보상 Grant ~*/

// 즉시 지급 단위. 보상 이벤트 발생 시 서버가 구성하여 오너 클라로 푸시
// 세션 종료 시 정산하는 배치 경로는 사용하지 않는다 (Logout은 Reliable RPC 도달 보장 X)
USTRUCT(BlueprintType)
struct FPRRewardGrant
{
	GENERATED_BODY()

	// 획득 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Experience = 0;

	// 획득 재화 (기본 통화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Currency = 0;

	// 아이템 드롭·재료 등은 Inventory 시스템 구현 시 필드 추가
};
