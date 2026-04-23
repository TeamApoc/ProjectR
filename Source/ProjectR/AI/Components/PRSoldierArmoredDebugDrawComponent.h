// Copyright ProjectR. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PRSoldierArmoredDebugDrawComponent.generated.h"

class UPRPatternDataAsset;
class UPRSoldierArmoredCombatDataAsset;
struct FPRPatternRule;

// Soldier_Armored PIE debug range draw
UCLASS(ClassGroup = (ProjectR), meta = (BlueprintSpawnableComponent))
class PROJECTR_API UPRSoldierArmoredDebugDrawComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Tick 기반 디버그 드로우 기본값 설정
	UPRSoldierArmoredDebugDrawComponent();

	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// 현재 PIE 환경에서 디버그 링을 그릴 수 있는지 확인
	bool ShouldDrawDebugRanges() const;

	// CombatDataAsset / PatternDataAsset 기반 거리 링 표시
	void DrawCombatRanges() const;

	// 현재 ThreatComponent가 선택한 타겟 반환
	AActor* GetCurrentTarget() const;

	// Owner 또는 Override에서 패턴 데이터 반환
	const UPRPatternDataAsset* GetPatternDataAsset() const;

	// SprintHammer pattern rule lookup
	const FPRPatternRule* FindSprintHammerRule(const UPRPatternDataAsset* PatternDataAsset) const;

	// Owner와 타겟 사이 거리 계산
	float GetDistanceToTarget(const AActor* CurrentTarget) const;

	// 수평 거리 링 표시
	void DrawRangeCircle(const FVector& Center, float Radius, const FColor& Color) const;

	// 거리 링 라벨 표시
	void DrawRangeLabel(const FString& Label, const FVector& Center, float Radius, const FColor& Color) const;

	// 단일 최대 거리 조건 링 표시
	void DrawMaxRange(const FString& Label, const FVector& Center, float Radius, float DistanceToTarget) const;

protected:
	// Debug draw enable flag
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bEnableRangeDebug = true;

	// 콘솔 변수 pr.EnemyAI.DebugSoldierArmoredRanges 필요 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bRequireConsoleVariable = true;

	// Authority instance only
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawOnlyAuthority = true;

	// Hammer combo and hit range data
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRSoldierArmoredCombatDataAsset> CombatDataAsset;

	// 패턴 데이터 직접 지정값. 비어 있으면 Owner의 PatternDataAsset 사용
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	TObjectPtr<UPRPatternDataAsset> PatternDataAssetOverride;

	// 링을 바닥에서 띄우는 높이
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawHeightOffset = 8.0f;

	// 링 선분 개수
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "12"))
	int32 CircleSegments = 96;

	// 링 두께
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float LineThickness = 2.0f;

	// Tick마다 다시 그릴 때의 표시 지속 시간
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug", meta = (ClampMin = "0.0"))
	float DrawDuration = 0.0f;

	// 조건 만족 색상
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor ActiveColor = FColor::Green;

	// 조건 불만족 색상
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor InactiveColor = FColor(160, 160, 160);

	// Sprint 구간 조건 만족 색상
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	FColor SprintActiveColor = FColor::Cyan;

	// 라벨 표시 여부
	UPROPERTY(EditAnywhere, Category = "ProjectR|Debug")
	bool bDrawLabels = true;
};
