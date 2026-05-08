// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRProjectileTrajectoryPreviewComponent.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UPRProjectileTrajectoryPreviewComponent::UPRProjectileTrajectoryPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

/*~ Public API ~*/

void UPRProjectileTrajectoryPreviewComponent::SetFireParams(const FPRProjectilePreviewParams& InParams)
{
	FireParams = InParams;
}

void UPRProjectileTrajectoryPreviewComponent::SetOriginComponent(USceneComponent* InOrigin)
{
	OriginComponent = InOrigin;

	// 댕글링 방지: 유효하지 않은 Origin 주입 시 표시 강제 OFF
	if (!IsValid(InOrigin) && bIsShowing)
	{
		Hide();
	}
}

void UPRProjectileTrajectoryPreviewComponent::Show()
{
	if (bIsShowing)
	{
		return;
	}

	bIsShowing = true;
	SetComponentTickEnabled(true);
}

void UPRProjectileTrajectoryPreviewComponent::Hide()
{
	if (!bIsShowing)
	{
		return;
	}

	bIsShowing = false;
	SetComponentTickEnabled(false);
	ClearTrajectory();
}

/*~ Draw ~*/

void UPRProjectileTrajectoryPreviewComponent::DrawTrajectory(const TArray<FVector>& SamplePoints)
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!IsValid(World) || SamplePoints.Num() == 0)
	{
		return;
	}

	const bool bImpactDetected = LastResult.EndReason == EPRPreviewEndReason::HitBlocking;
	const int32 LastIndex = SamplePoints.Num() - 1;

	for (int32 i = 0; i < SamplePoints.Num(); ++i)
	{
		// 착탄 사유로 종료된 경우에 한해 마지막 포인트만 적색으로 강조
		const bool bIsImpactPoint = bImpactDetected && (i == LastIndex);
		const FColor PointColor = bIsImpactPoint ? DebugHitColor : DebugColor;

		// 매 틱 갱신되므로 LifeTime 0(단발 프레임) 사용
		DrawDebugSphere(World, SamplePoints[i], DebugSphereRadius, 8, PointColor, false, 0.f, 0, 0.f);
	}
#endif
}

/*~ Tick ~*/

void UPRProjectileTrajectoryPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Origin 유효성 재확인. 무기 교체 등으로 사라졌을 수 있음
	if (!OriginComponent.IsValid())
	{
		Hide();
		return;
	}

	RebuildPath();
	DrawTrajectory(LastResult.SamplePoints);
}

/*~ Path Build ~*/

void UPRProjectileTrajectoryPreviewComponent::RebuildPath()
{
	LastResult.SamplePoints.Reset();
	LastResult.EndReason = EPRPreviewEndReason::None;
	LastResult.EndHit = FHitResult();
	LastResult.ElapsedSimTime = 0.f;

	USceneComponent* Origin = OriginComponent.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Origin) || !IsValid(World))
	{
		return;
	}

	const FVector StartLocation = Origin->GetComponentLocation();
	const FVector LaunchVelocity = Origin->GetForwardVector() * FireParams.InitialSpeed;

	FPredictProjectilePathParams PathParams(FireParams.CollisionRadius, StartLocation, LaunchVelocity, FireParams.MaxSimTime);
	PathParams.bTraceWithCollision = true;
	PathParams.bTraceWithChannel = true;
	PathParams.TraceChannel = FireParams.TraceChannel;
	PathParams.SimFrequency = FireParams.SimFrequency;

	// 중력 스케일 적용. GravityScale이 0에 가까운 경우 OverrideGravityZ==0이 "월드 기본 사용"으로 해석되므로 매우 작은 값으로 보정
	const float WorldGravityZ = World->GetGravityZ();
	float OverrideGravityZ = WorldGravityZ * FireParams.GravityScale;
	if (FMath::IsNearlyZero(OverrideGravityZ))
	{
		OverrideGravityZ = KINDA_SMALL_NUMBER;
	}
	PathParams.OverrideGravityZ = OverrideGravityZ;

	// 자기 액터는 충돌에서 제외하여 발사 직후 자기 콜리전에 막히는 케이스 방지
	if (AActor* OwnerActor = GetOwner())
	{
		PathParams.ActorsToIgnore.Add(OwnerActor);
	}

	FPredictProjectilePathResult PathResult;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// 다운샘플링: 누적 거리 기준 SampleSpacing 간격으로 포인트 추출. MaxDistance / MaxSamplePoints 도달 시 조기 종료
	const TArray<FPredictProjectilePathPointData>& Path = PathResult.PathData;
	if (Path.Num() == 0)
	{
		LastResult.EndReason = bHit ? EPRPreviewEndReason::HitBlocking : EPRPreviewEndReason::TimeExceeded;
		if (bHit)
		{
			LastResult.EndHit = PathResult.HitResult;
		}
		return;
	}

	const bool bHasDistanceCap = FireParams.MaxDistance > 0.f;
	const bool bHasCountCap = FireParams.MaxSamplePoints > 0;
	const float Spacing = FMath::Max(1.f, FireParams.SampleSpacing);

	// 시작점은 항상 포함
	LastResult.SamplePoints.Add(Path[0].Location);

	float AccumulatedDistance = 0.f;
	float DistanceSinceLastSample = 0.f;
	bool bDistanceCapped = false;
	FVector PrevLocation = Path[0].Location;
	FVector LastSampledLocation = Path[0].Location;

	for (int32 i = 1; i < Path.Num(); ++i)
	{
		const FVector Current = Path[i].Location;
		const float SegmentLength = FVector::Dist(PrevLocation, Current);

		// 거리 제한 도달 시 보간 지점에서 종료
		if (bHasDistanceCap && AccumulatedDistance + SegmentLength > FireParams.MaxDistance)
		{
			const float Remaining = FireParams.MaxDistance - AccumulatedDistance;
			const float Alpha = SegmentLength > KINDA_SMALL_NUMBER ? Remaining / SegmentLength : 0.f;
			const FVector EndPoint = FMath::Lerp(PrevLocation, Current, Alpha);
			LastResult.SamplePoints.Add(EndPoint);
			LastSampledLocation = EndPoint;
			AccumulatedDistance = FireParams.MaxDistance;
			bDistanceCapped = true;
			break;
		}

		AccumulatedDistance += SegmentLength;
		DistanceSinceLastSample += SegmentLength;

		if (DistanceSinceLastSample >= Spacing)
		{
			LastResult.SamplePoints.Add(Current);
			LastSampledLocation = Current;
			DistanceSinceLastSample = 0.f;

			if (bHasCountCap && LastResult.SamplePoints.Num() >= FireParams.MaxSamplePoints)
			{
				break;
			}
		}

		PrevLocation = Current;
	}

	// 끝점이 마지막 샘플과 다르고 거리/개수 제한에 걸리지 않았다면 종료점 추가하여 표시 끊김 방지
	if (!bDistanceCapped)
	{
		const FVector PathEnd = Path.Last().Location;
		const bool bCountCapReached = bHasCountCap && LastResult.SamplePoints.Num() >= FireParams.MaxSamplePoints;
		if (!bCountCapReached && !LastSampledLocation.Equals(PathEnd, KINDA_SMALL_NUMBER))
		{
			LastResult.SamplePoints.Add(PathEnd);
		}
	}

	// 종료 사유 결정
	LastResult.ElapsedSimTime = Path.Last().Time;
	if (bHit && !bDistanceCapped)
	{
		LastResult.EndReason = EPRPreviewEndReason::HitBlocking;
		LastResult.EndHit = PathResult.HitResult;
	}
	else if (bDistanceCapped)
	{
		LastResult.EndReason = EPRPreviewEndReason::DistanceExceeded;
	}
	else
	{
		LastResult.EndReason = EPRPreviewEndReason::TimeExceeded;
	}
}
