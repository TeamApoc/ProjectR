// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectR/AI/Boss/PRBossPatternActor.h"
#include "PRBossPortalActor.generated.h"

// 보스 포털 패턴의 공용 Helper Actor다.
// 실제 Mesh/VFX/Projectile은 BP 파생에서 붙이고, C++는 텔레그래프와 활성/만료 생명주기만 관리한다.
UCLASS(Blueprintable)
class PROJECTR_API APRBossPortalActor : public APRBossPatternActor
{
	GENERATED_BODY()

public:
	APRBossPortalActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 현재 포털이 발사/위험 상태인지 반환한다.
	UFUNCTION(BlueprintPure, Category = "ProjectR|AI|Boss|Portal")
	bool IsPortalActive() const { return bPortalActive; }

	// 서버에서 포털 텔레그래프를 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void StartPortalTelegraph();

	// 서버에서 포털을 활성화한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ActivatePortal();

	// 서버에서 포털을 만료 처리한다.
	UFUNCTION(BlueprintCallable, Category = "ProjectR|AI|Boss|Portal")
	void ExpirePortal();

protected:
	virtual void BeginPlay() override;

	// 모든 클라이언트에서 텔레그래프 시작 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalTelegraphStarted();

	// 모든 클라이언트에서 포털 활성 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalActivated();

	// 모든 클라이언트에서 포털 만료 연출을 실행한다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPortalExpired();

	// 포털 텔레그래프 시작 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalTelegraphStarted();

	// 포털 활성 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalActivated();

	// 포털 만료 BP 이벤트다.
	UFUNCTION(BlueprintImplementableEvent, Category = "ProjectR|AI|Boss|Portal")
	void BP_OnPortalExpired();

protected:
	// BeginPlay에서 자동으로 텔레그래프를 시작할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bAutoStartPortal = true;

	// 텔레그래프 시작 후 실제 활성까지 걸리는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActivationDelay = 0.75f;

	// 활성 후 만료까지 유지되는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal", meta = (ClampMin = "0.0"))
	float ActiveDuration = 1.5f;

	// 만료 이벤트 후 Actor를 제거할지 여부다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bDestroyWhenExpired = true;

	// 현재 포털 활성 상태다.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ProjectR|AI|Boss|Portal")
	bool bPortalActive = false;

private:
	FTimerHandle PortalActivationTimerHandle;
	FTimerHandle PortalExpireTimerHandle;
};
