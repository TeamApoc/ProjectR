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
	Right,
	Backward,
	Left,
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