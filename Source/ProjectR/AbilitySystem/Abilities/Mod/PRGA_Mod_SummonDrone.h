// Copyright ProjectR. All Rights Reserved.
// Author: 이건주 (Mod Summon Drone 구현)
#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Mod/PRGA_Mod_HasDuration.h"
#include "PRGA_Mod_SummonDrone.generated.h"

class APRSupportDroneActor;
class UPRSupportDroneDataAsset;

// 보조 드론을 소환하는 모드 어빌리티다
UCLASS()
class PROJECTR_API UPRGA_Mod_SummonDrone : public UPRGA_Mod_HasDuration
{
	GENERATED_BODY()

public:
	// 드론 소환 모드 기본 설정
	UPRGA_Mod_SummonDrone();

	/*~ UGameplayAbility Interface ~*/
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	/*~ UPRGA_Mod_HasDuration Interface ~*/
	// 지속시간 시작 시 서버 드론 소환
	virtual void OnDurationStarted_Implementation() override;

	/*~ UPRGA_Mod Interface ~*/
	// 어빌리티 회수 시 드론 런타임 정리
	virtual void CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	// 서버에서 드론을 생성한다
	APRSupportDroneActor* SpawnSupportDrone(const FGameplayAbilityActorInfo* ActorInfo);

	// 기존 드론을 제거한다
	void DestroyActiveDrone();

protected:
	// 소환할 드론 클래스다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Drone")
	TSubclassOf<APRSupportDroneActor> DroneClass;

	// 소환할 드론 데이터다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Drone")
	TObjectPtr<UPRSupportDroneDataAsset> DroneData;

	// 플레이어 기준 초기 소환 위치다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Drone")
	FVector SpawnOffset = FVector(-120.0f, 90.0f, 170.0f);

	// 기존 드론 교체 여부다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|Mod|Drone")
	bool bReplaceExistingDrone = true;

private:
	// 현재 서버에서 유지 중인 드론
	UPROPERTY(Transient)
	TObjectPtr<APRSupportDroneActor> ActiveDrone;
};
