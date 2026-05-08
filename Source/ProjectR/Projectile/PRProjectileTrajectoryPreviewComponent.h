// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRProjectileTypes.h"
#include "PRProjectileTrajectoryPreviewComponent.generated.h"

class USceneComponent;

/**
 * 투사체 발사 예측 경로를 매 틱 산출하는 액터 컴포넌트.
 * 시각화는 DrawTrajectory 가상 함수로 위임하며, 실제 출력 방식은 파생 클래스에서 구현.
 * 시각화는 로컬 클라이언트 전용으로 복제되지 않음.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTR_API UPRProjectileTrajectoryPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPRProjectileTrajectoryPreviewComponent();

	// 발사 파라미터 일괄 갱신. 런타임 호출 가능
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void SetFireParams(const FPRProjectilePreviewParams& InParams);

	// 기준 SceneComponent 주입. nullptr 주입 시 표시는 강제 OFF로 전환
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void SetOriginComponent(USceneComponent* InOrigin);

	// 표시 활성화. PrimaryComponentTick 활성. 이미 활성이면 무시
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void Show();

	// 표시 비활성화. PrimaryComponentTick 비활성 + ClearTrajectory 호출. 이미 비활성이면 무시
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void Hide();

	// 표시 상태 조회
	UFUNCTION(BlueprintPure, Category = "ProjectR|Projectile|Preview")
	bool IsShowing() const { return bIsShowing; }

	// 직전 틱 산출 결과 조회. UI/에임 어시스트 등 외부 시스템이 착탄 위치 참조 시 사용
	const FPRProjectilePreviewResult& GetLastResult() const { return LastResult; }

protected:
	/*~ Draw Interface ~*/

	// 매 틱 산출된 샘플 포인트 배열로 궤적을 출력. 베이스 구현은 DrawDebugSphere로 각 포인트에 작은 구체 표시(임시 시각화).
	// 출력 방식 확정 시 파생 클래스에서 오버라이드
	virtual void DrawTrajectory(const TArray<FVector>& SamplePoints);

	// 표시 종료 시 출력 상태를 초기화. 베이스 구현은 매 틱 단발 디버그 드로우만 사용하므로 별도 정리 불필요
	virtual void ClearTrajectory() {}

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// PredictProjectilePath 1회 호출 + 결과를 SampleSpacing 기준으로 다운샘플링하여 LastResult 갱신
	void RebuildPath();

protected:
	// 발사 파라미터 묶음. 무기/탄종 변경 시 일괄 교체
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	FPRProjectilePreviewParams FireParams;

	// 임시 DrawDebugSphere 시각화의 구체 반지름(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug", meta = (ClampMin = "0.1"))
	float DebugSphereRadius = 5.f;

	// 일반 포인트 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
	FColor DebugColor = FColor::Green;

	// 착탄 지점 강조 색상. EndReason이 HitBlocking일 때 마지막 포인트에 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
	FColor DebugHitColor = FColor::Red;

private:
	// 궤적 기점 컴포넌트 약참조. 매 틱 GetComponentTransform()으로 시작 위치/방향 조회
	TWeakObjectPtr<USceneComponent> OriginComponent;

	// 표시 ON/OFF 상태
	bool bIsShowing = false;

	// 직전 틱 산출 결과 캐시
	FPRProjectilePreviewResult LastResult;
};
