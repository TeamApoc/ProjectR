// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PRFootstepSoundDataAsset.generated.h"

class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

UENUM(BlueprintType)
enum class EPRFootstepFoot : uint8
{
	Left,
	Right,
};

// 표면 하나에 대응하는 발소리 설정
USTRUCT(BlueprintType)
struct PROJECTR_API FPRFootstepSoundEntry
{
	GENERATED_BODY()

	// 재생할 사운드 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TObjectPtr<USoundBase> Sound = nullptr;

	// 재생 볼륨 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	// 재생 피치 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio", meta = (ClampMin = "0.1"))
	float PitchMultiplier = 1.0f;

	// 원격 플레이어 3D 재생용 감쇠 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;

	// 동시 재생 제한 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TObjectPtr<USoundConcurrency> ConcurrencySettings = nullptr;

	// AI 청각 자극 세기 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|AI", meta = (ClampMin = "0.0"))
	float NoiseLoudnessMultiplier = 1.0f;
};

// 표면 재질별 발소리와 발 소켓 이름을 담는 데이터 에셋
UCLASS(BlueprintType)
class PROJECTR_API UPRFootstepSoundDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 발 구분에 대응하는 소켓 이름 반환
	FName ResolveFootSocketName(EPRFootstepFoot Foot) const;

	// 표면 타입에 대응하는 발소리 설정 반환
	const FPRFootstepSoundEntry* FindSoundEntry(EPhysicalSurface SurfaceType) const;

public:
	// 매핑 실패 또는 기본 표면용 발소리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	FPRFootstepSoundEntry DefaultEntry;

	// 표면 타입별 발소리 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	TMap<TEnumAsByte<EPhysicalSurface>, FPRFootstepSoundEntry> SurfaceEntries;

	// 왼발 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Socket")
	FName LeftFootSocketName = NAME_None;

	// 오른발 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Socket")
	FName RightFootSocketName = NAME_None;

	// 트레이스 실패 시 기본 사운드 재생 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Footstep|Audio")
	bool bPlayDefaultWhenTraceMissed = true;
};
