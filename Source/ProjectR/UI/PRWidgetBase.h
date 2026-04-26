// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PRWidgetBase.generated.h"

class UPBViewModelBase;

/**
 * 위젯이 요구하는 입력 모드
 */
UENUM(BlueprintType)
enum class EPBUIInputMode : uint8
{
	None,       // 입력 모드 변경 안함 (HUD, 데미지 넘버)
	GameAndUI,  // 카메라 이동 가능 (인벤토리, 캐릭터 시트)
	UIOnly,     // 게임 입력 차단 (대화, 레벨업)
};

/**
 * UI 레이어. ZOrder 및 스택 우선순위에 사용된다.
 * 값이 높을수록 위에 표시되며, 입력 모드 결정 시 우선한다.
 */
UENUM(BlueprintType)
enum class EPRUILayer : uint8
{
	HUD       = 0,  // 게임플레이 HUD (체력, 탄약, 크로스헤어)
	Menu      = 1,  // 인벤토리, 캐릭터 시트, 일시정지
	Modal     = 2,  // 다이얼로그, 확인창
	System    = 3,  // 로딩, 치명적 에러 등 최상위
};

/**
 * UI 위젯 기본 클래스
 * Push/Pop 스택에서 관리되는 모든 위젯의 부모 클래스
 */
UCLASS(Abstract, BlueprintType)
class PROJECTR_API UPRWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 위젯이 속한 레이어. UIManager가 ZOrder/스택 정렬에 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPRUILayer Layer = EPRUILayer::Menu;

	// 이 위젯이 활성화될 때 적용할 입력 모드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPBUIInputMode InputMode = EPBUIInputMode::GameAndUI;

	// 이 위젯이 활성화될 때 마우스 커서 표시 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bShowMouseCursor = true;
};

/** TODO [고도화]:
 *  - Push/Pop 진입/퇴장 애니메이션
 *  - ESC 키 닫기 정책 (bClosableByEsc)
 *  - 배경 블러/딤 설정 (Modal 레이어 활성 시 하위 레이어 블러 등)
 */