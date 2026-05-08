// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRProjectileTypes.h"
#include "PRProjectileTrajectoryPreviewComponent.generated.h"

class APRWeaponActor;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;


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

	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void SetPreviewEnabled(bool bInEnabled);
	
	// 발사 파라미터 일괄 갱신. 런타임 호출 가능
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void SetFireParams(const FPRProjectilePreviewParams& InParams);

	// 궤적 기점 무기 액터 주입. nullptr 주입 시 표시 강제 OFF
	UFUNCTION(BlueprintCallable, Category = "ProjectR|Projectile|Preview")
	void SetWeaponActor(APRWeaponActor* InWeaponActor);

	// 표시 활성화. PrimaryComponentTick 활성. WeaponActor가 무효이면 활성화 무시
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

	// 매 틱 산출된 샘플 포인트 배열로 궤적을 출력. 베이스 구현은 DrawDebugSphere로 각 포인트 표시 후 끝에서 DrawTrajectoryHISM 호출.
	// 정식 백엔드(HISM)는 PreviewMesh 지정 시 활성, 미지정 시 자동 스킵
	virtual void DrawTrajectory(const TArray<FVector>& SamplePoints);

	// 표시 종료 시 출력 상태를 초기화. 디버그 드로우는 단발 프레임이므로 정리 불필요, HISM 인스턴스만 비움
	virtual void ClearTrajectory();

	// HISM 인스턴스를 SamplePoints에 맞춰 갱신. 일반 포인트는 CustomData=0, 착탄 포인트는 CustomData=1로 설정.
	// PreviewMesh 미지정 또는 owner 액터 부재 시 no-op
	virtual void DrawTrajectoryHISM(const TArray<FVector>& SamplePoints);

	// HISM 인스턴스 일괄 제거
	virtual void ClearTrajectoryHISM();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnUnregister() override;

private:
	// PredictProjectilePath 1회 호출 + 결과를 SampleSpacing 기준으로 다운샘플링하여 LastResult 갱신
	void RebuildPath();

protected:
	// 발사 파라미터 묶음. 무기/탄종 변경 시 일괄 교체
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview")
	FPRProjectilePreviewParams FireParams;

	// 임시 DrawDebugSphere 시각화의 구체 반지름(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug", meta = (ClampMin = "0.1"))
	float DebugSphereRadius = 3.f;
	
	// 일반 포인트 색상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
	FColor DebugColor = FColor::Green;

	// 착탄 지점 강조 색상. EndReason이 HitBlocking일 때 마지막 포인트에 적용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|Debug")
	FColor DebugHitColor = FColor::Red;

	// HISM 인스턴싱에 사용할 작은 구체 메시. nullptr이면 HISM 출력 자동 스킵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|HISM")
	TObjectPtr<UStaticMesh> PreviewMesh;

	// HISM 인스턴스의 균일 스케일 배율. 메시 원본 스케일 기준
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProjectR|Projectile|Preview|HISM", meta = (ClampMin = "0.01"))
	float PreviewMeshScale = 1.f;

private:
	// 궤적 기점 무기 액터 약참조. 매 틱 GetMuzzleTransform()으로 시작 위치/방향 조회
	TWeakObjectPtr<APRWeaponActor> WeaponActor;

	// HISM 컴포넌트. owner 액터에 동적 생성/등록되며 OnUnregister에서 정리.
	// 첫 DrawTrajectoryHISM 호출 시 lazy 생성
	UPROPERTY(Transient)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TrajectoryHISM;

	// 활성화 여부
	bool bIsEnabled = false;
	
	// 표시 ON/OFF 상태
	bool bIsShowing = false;

	// 직전 틱 산출 결과 캐시
	FPRProjectilePreviewResult LastResult;
};
