// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "PRCameraManager.generated.h"

/**
 * FOV 트랜지션과 숄더스왑 로직의 중앙 제어소 역할을 합니다.
 */
UCLASS()
class PROJECTR_API APRCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	APRCameraManager();
	// 모디파이어(구르기 등)가 외부에서 덮어씌울 목표 FOV (0이면 사용 안 함)
	float ModifierTargetFOV = 0.0f;
	
	// 어빌리티가 조준 중일 때 덮어씌울 목표 FOV (0.0f이면 조준 안 함)
	float OverrideAimFOV = 0.0f;
protected:
	/*~ APlayerCameraManager Interface ~*/
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

private:
	// 보간을 위한 내부 변수
	float TargetFOV = 80.0f;
	
	// 보간을 위해 현재 FOV 값을 스스로 기억하는 변수
	float CurrentFOV = 80.0f;
};
