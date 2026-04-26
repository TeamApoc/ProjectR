// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PRWeaponDataAsset.generated.h"

class APRWeaponActor;
class USkeletalMesh;

// 1차 무기 장착 공개 상태 검증에 사용하는 최소 무기 데이터다.
UCLASS(BlueprintType)
class PROJECTR_API UPRWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPRWeaponDataAsset();

public:
	// 디버그와 UI에서 구분할 무기 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	FName WeaponId;

	// 슬롯별 공개 무기 표현에 사용할 Actor 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TSubclassOf<APRWeaponActor> WeaponActorClass;

	// 테스트용 로컬 무기 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Weapon")
	TObjectPtr<USkeletalMesh> DisplayMesh;
};
