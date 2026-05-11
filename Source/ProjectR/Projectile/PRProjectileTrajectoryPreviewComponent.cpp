// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRProjectileTrajectoryPreviewComponent.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"

UPRProjectileTrajectoryPreviewComponent::UPRProjectileTrajectoryPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.016f;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPRProjectileTrajectoryPreviewComponent::SetPreviewEnabled(bool bInEnabled)
{
	bIsEnabled = bInEnabled;
}

/*~ Public API ~*/

void UPRProjectileTrajectoryPreviewComponent::SetFireParams(const FPRProjectilePreviewParams& InParams)
{
	FireParams = InParams;
}

void UPRProjectileTrajectoryPreviewComponent::SetWeaponActor(APRWeaponActor* InWeaponActor)
{
	WeaponActor = InWeaponActor;

	// 댕글링 방지: 무효 WeaponActor 주입 시 표시 강제 OFF
	if (!IsValid(InWeaponActor) && bIsShowing)
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

	// WeaponActor가 무효이면 매 틱 즉시 Hide로 빠지므로 활성화 자체를 무시
	if (!WeaponActor.IsValid())
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
	if (bDrawDebug)
	{
		UWorld* World = GetWorld();
		if (IsValid(World) && SamplePoints.Num() > 0)
		{
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
		}
	}
#endif

	// PreviewMesh가 지정된 경우에만 동작
	DrawTrajectoryHISM(SamplePoints);
}

void UPRProjectileTrajectoryPreviewComponent::ClearTrajectory()
{
	ClearTrajectoryHISM();
}

void UPRProjectileTrajectoryPreviewComponent::DrawTrajectoryHISM(const TArray<FVector>& SamplePoints)
{
	if (!IsValid(PreviewMesh))
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// Lazy 생성. 첫 호출 시 owner 액터에 HISM을 동적 생성/등록.
	// 런타임 SceneComponent 생성 표준 패턴: NewObject 후 Mobility 설정과 SetupAttachment를 RegisterComponent 이전에 수행
	if (!IsValid(TrajectoryHISM))
	{
		TrajectoryHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
		TrajectoryHISM->SetMobility(EComponentMobility::Movable);
		if (USceneComponent* OwnerRoot = Owner->GetRootComponent())
		{
			TrajectoryHISM->SetupAttachment(OwnerRoot);
		}
		TrajectoryHISM->SetStaticMesh(PreviewMesh);
		TrajectoryHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TrajectoryHISM->SetGenerateOverlapEvents(false);
		TrajectoryHISM->SetCastShadow(false);
		// 머티리얼이 PerInstanceCustomData를 샘플링하여 일반/착탄 색상 분기 가능하도록 1채널 확보
		TrajectoryHISM->SetNumCustomDataFloats(1);
		// 인스턴스 변경 시 HISM 내부 트리 자동 재구성을 보장
		TrajectoryHISM->bAutoRebuildTreeOnInstanceChanges = true;
		TrajectoryHISM->RegisterComponent();
	}

	// In-Place 갱신. ClearInstances를 사용하면 인스턴스가 0개인 프레임이 렌더되어 깜빡임 발생.
	// 기존 인스턴스의 Transform만 갱신하고, 수량 차이만 Add/Remove로 보정
	const int32 NewCount = SamplePoints.Num();
	const int32 OldCount = TrajectoryHISM->GetInstanceCount();

	const bool bImpactDetected = LastResult.EndReason == EPRPreviewEndReason::HitBlocking;
	const int32 LastIndex = NewCount - 1;
	const FVector InstanceScale(PreviewMeshScale);

	// 기존 인스턴스 Transform 갱신 (공통 범위)
	const int32 UpdateCount = FMath::Min(OldCount, NewCount);
	for (int32 i = 0; i < UpdateCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		TrajectoryHISM->UpdateInstanceTransform(i, InstanceTransform, /*bWorldSpace=*/true,
			/*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryHISM->SetCustomDataValue(i, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 부족한 만큼 추가
	for (int32 i = OldCount; i < NewCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		const int32 InstanceIndex = TrajectoryHISM->AddInstance(InstanceTransform, /*bWorldSpace=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryHISM->SetCustomDataValue(InstanceIndex, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 초과분 제거 (뒤에서부터)
	for (int32 i = OldCount - 1; i >= NewCount; --i)
	{
		TrajectoryHISM->RemoveInstance(i);
	}

	TrajectoryHISM->MarkRenderStateDirty();
}

void UPRProjectileTrajectoryPreviewComponent::ClearTrajectoryHISM()
{
	if (IsValid(TrajectoryHISM))
	{
		TrajectoryHISM->ClearInstances();
	}
}

void UPRProjectileTrajectoryPreviewComponent::OnUnregister()
{
	if (IsValid(TrajectoryHISM))
	{
		TrajectoryHISM->DestroyComponent();
		TrajectoryHISM = nullptr;
	}
	Super::OnUnregister();
}

/*~ Tick ~*/

void UPRProjectileTrajectoryPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// WeaponActor 유효성 재확인. 무기 교체 등으로 사라졌을 수 있음
	if (!WeaponActor.IsValid() || !bIsEnabled)
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

	APRWeaponActor* Weapon = WeaponActor.Get();
	UWorld* World = GetWorld();
	if (!IsValid(Weapon) || !IsValid(World))
	{
		return;
	}

	// 무기 머즐 트랜스폼을 시작 기준으로 사용. BP에서 override 가능
	const FTransform MuzzleTransform = Weapon->GetMuzzleTransform();
	const FVector StartLocation = MuzzleTransform.GetLocation();

	// 발사 방향은 카메라 조준점 기준으로 산출하여 실제 발사 흐름(UPRGameplayStatics::ResolveProjectileLaunchTransform)과 정합 유지.
	// 카메라 트레이스 거리는 항상 충분히 길게 잡아 원거리 조준점도 포착(MaxDistance와 별개)
	constexpr float AimTraceDistance = 20000.f;
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	TArray<AActor*> IgnoredActors;
	if (AActor* OwnerActor = GetOwner())
	{
		IgnoredActors.Add(OwnerActor);
	}
	IgnoredActors.Add(Weapon);

	const FTransform LaunchTransform = UPRGameplayStatics::ResolveProjectileLaunchTransform(OwnerPawn, StartLocation, AimTraceDistance, FireParams.TraceChannel, IgnoredActors);
	const FVector LaunchVelocity = LaunchTransform.GetRotation().GetForwardVector() * FireParams.InitialSpeed;

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

	// 자기 액터와 무기 액터는 충돌에서 제외하여 발사 직후 머즐 콜리전에 막히는 케이스 방지
	PathParams.ActorsToIgnore = IgnoredActors;

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

