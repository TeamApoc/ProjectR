// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "ProjectR/Game/PRCameraManager.h"

#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Player/PRCameraModifier_Recoil.h"

APRCameraManager::APRCameraManager()
{
	// 기본 카메라 설정
	DefaultFOV = 80.0f;
	ViewPitchMin = -80.0f;
	ViewPitchMax = 80.0f;
	bAlwaysApplyModifiers = true;
}

void APRCameraManager::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningController = GetOwningPlayerController();
	if (IsValid(OwningController) && OwningController->IsLocalController())
	{
		AddNewCameraModifier(UPRCameraModifier_Recoil::StaticClass());
	}
}

void APRCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);
	
	// 현재 카메라가 바라보고 있는 타겟이 PRPlayerCharacter인지 확인합니다.
	APRPlayerCharacter* PRCharacter = Cast<APRPlayerCharacter>(OutVT.Target);
	if (IsValid(PRCharacter))
	{
		// 캐릭터의 상태에 따라 TargetFOV를 결정합니다.
		TargetFOV = DefaultFOV;
		
		// 캐릭터가 실제로 이동 중인지 확인
		bool bIsMoving = PRCharacter->GetVelocity().Size2D() > 10.0f;
		
		if (OverrideAimFOV > 0.0f)
		{
			TargetFOV = OverrideAimFOV;
		}
		// 질주 상태 & 이동 중일 때
		else if (PRCharacter->IsSprinting() && bIsMoving)
		{
			TargetFOV = 90.0f; // 역동감을 위해 시야각을 넓힘
		}
		else if (PRCharacter->IsWalking())
		{
			TargetFOV = 60.0f; // 역동감을 위해 시야각을 넓힘
		}
		
		if (ModifierTargetFOV > 0.0f)
		{
			TargetFOV = ModifierTargetFOV;
		}

		// 3. 현재 FOV에서 목표 FOV로 부드럽게 보간합니다 (InterpSpeed: 10.0f).
		CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 10.0f);
		OutVT.POV.FOV = CurrentFOV;
		
		// 사용이 끝난 모디파이어 값은 매 프레임 초기화 (모디파이어가 비활성화되면 0이 됨)
		ModifierTargetFOV = 0.0f;
	}
}
