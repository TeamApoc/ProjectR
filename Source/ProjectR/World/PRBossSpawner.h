// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PRBossSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class APRBossBaseCharacter;
struct FGameplayTag;
struct FInstancedStruct;

/**
 * 보스 스폰 방법
 *
 * 1. bAutoSpawn이 활성화된 경우 BeginPlay에서 서버 권한으로 자동 스폰
 * 2. SpawnBossCharacter를 직접 호출하는 경우 Blueprint 또는 C++에서 즉시 스폰
 * 3. EventManager가 Event.Boss.Spawn을 BroadcastEmpty로 발행한 경우 등록된 APRBossSpawner가 수신 후 스폰
 */
UCLASS()
class PROJECTR_API APRBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	APRBossSpawner();

	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ APRBossSpawner Interface ~*/
	// 보스 캐릭터 스폰 실행
	UFUNCTION(BlueprintCallable)
	APRBossBaseCharacter* SpawnBossCharacter();
	
private:
	// 보스 스폰 이벤트 수신 처리
	void HandleBossSpawnEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PR|Spawn")
	TSubclassOf<APRBossBaseCharacter> BossCharacterClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PR|Spawn")
	bool bAutoSpawn = true;
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UArrowComponent> Arrow;
	
	UPROPERTY(VisibleInstanceOnly)
	TWeakObjectPtr< APRBossBaseCharacter > SpawnedCharacter;

	// 보스 스폰 이벤트 구독 핸들
	FDelegateHandle BossSpawnEventHandle;
};
