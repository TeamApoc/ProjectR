// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "ProjectR/Player/Components/PRSpringArmComponent.h"

#include "ProjectR/Character/PRPlayerCharacter.h"

UPRSpringArmComponent::UPRSpringArmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPRSpringArmComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	// 이 컴포넌트를 소유한 액터가 PRPlayerCharacter인지 확인합니다.
	APRPlayerCharacter* PRCharacter = Cast<APRPlayerCharacter>(GetOwner());

	// 소유자가 유효하고, 그 캐릭터가 로컬에서 조종 중일 때만 연산합니다.
	if (IsValid(PRCharacter) && PRCharacter->IsLocallyControlled())
	{
		// 목표값 설정 
		float TargetLength = 300.0f;
		// 1. 회전축(TargetOffset)은 무조건 정중앙 머리 위 (Y=0)
		FVector Dest_TargetOffset = FVector(0.0f, 0.0f, 80.0f);

		// 2. 어깨너머 시점(SocketOffset)만 우측으로 밀기 (Y=60)
		FVector Dest_SocketOffset = FVector(0.0f, 70.0f, 0.0f);
		
		// 현재 캐릭터가 실제로 이동 중인지 확인
		bool bIsMoving = PRCharacter->GetVelocity().Size2D() > 10.0f;
		
		if (bIsAimingOverride)
		{
			TargetLength = AimTargetArmLength;
			Dest_TargetOffset = AimTargetOffset;
			Dest_SocketOffset = AimSocketOffset; 
		}

		// 현재 값에서 목표 값으로 부드럽게 보간
		TargetArmLength = FMath::FInterpTo(TargetArmLength, TargetLength, DeltaTime, ArmLengthInterpSpeed);
		TargetOffset = FMath::VInterpTo(TargetOffset, Dest_TargetOffset, DeltaTime, SocketOffsetInterpSpeed);
		SocketOffset = FMath::VInterpTo(SocketOffset, Dest_SocketOffset, DeltaTime, SocketOffsetInterpSpeed);
	}

	// 보간이 완료된 후 엔진의 기본 붐 충돌 및 트랜스폼 계산 로직을 실행합니다.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
