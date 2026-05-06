// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AbilitySystem/Abilities/Boss/PRGameplayAbility_BossSpawnPatternActors.h"
#include "PRGameplayAbility_BossPortalSequence.generated.h"

class APRBossPortalActor;

// Faerin 계열 포털 패턴의 실행 타입이다.
UENUM(BlueprintType)
enum class EPRBossPortalPatternType : uint8
{
	Missile		UMETA(DisplayName = "Missile"),
	Barrage		UMETA(DisplayName = "Barrage"),
	Attached	UMETA(DisplayName = "Attached"),
	Torrent		UMETA(DisplayName = "Torrent")
};

// 보스 포털 Helper Actor를 생성하고 Ability 종료/취소 시 정리하는 공용 패턴 Ability다.
UCLASS()
class PROJECTR_API UPRGameplayAbility_BossPortalSequence : public UPRGameplayAbility_BossSpawnPatternActors
{
	GENERATED_BODY()

public:
	UPRGameplayAbility_BossPortalSequence();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// 포털 시퀀스 유지 시간이 끝났을 때 Ability를 정상 종료한다.
	void FinishPortalSequence();

	// Ability가 보유한 포털 Actor를 만료 처리한다.
	void ExpireSpawnedPortals();

protected:
	// 이번 포털 시퀀스의 패턴 타입이다. 1차에서는 BP/데이터 분기와 디버그 기준으로 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	EPRBossPortalPatternType PortalPatternType = EPRBossPortalPatternType::Missile;

	// 포털 생성 직후 텔레그래프를 시작할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bStartPortalsAfterSpawn = true;

	// Ability 종료/취소 시 생성한 포털을 만료 처리할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bExpireSpawnedPortalsOnEnd = true;

	// 포털 시퀀스 유지 시간이다. 0 이하이면 포털 생성 직후 Ability를 종료한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float PortalSequenceDuration = 15.0f;

	// 이번 Ability 실행에서 생성한 포털 Actor 목록이다.
	UPROPERTY(BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	TArray<TObjectPtr<APRBossPortalActor>> SpawnedPortalRefs;

private:
	FTimerHandle PortalSequenceTimerHandle;
};
