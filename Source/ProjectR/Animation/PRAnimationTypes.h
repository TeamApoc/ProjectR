// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "PRAnimationTypes.generated.h"

// 캐릭터 착지 강도
UENUM(BlueprintType)
enum class ELandState : uint8
{
	None,
	Normal,
	Soft,
	Heavy,
};

// 캐릭터 이동 방향
UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	Forward,
	ForwardRight,
	ForwardLeft,
	Backward,
	BackwardRight,
	BackwardLeft
};

// 캐릭터 이동 모드 (애니메이션 판별용)
UENUM(BlueprintType)
enum class EPRMovementMode : uint8
{
	Idle,
	Walking,
	Jogging,
	Sprinting
};

// 캐릭터 턴 각도
UENUM(BlueprintType)
enum class EPRTurnAngle : uint8
{
	None,
	Angle90
};