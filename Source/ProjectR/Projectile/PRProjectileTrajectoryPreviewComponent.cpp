// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#include "PRProjectileTrajectoryPreviewComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
#include "ProjectR/ItemSystem/Actors/PRWeaponActor.h"
#include "ProjectR/ItemSystem/Components/PRWeaponManagerComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPRPathPreviewComponent, Log, All);

UPRProjectileTrajectoryPreviewComponent::UPRProjectileTrajectoryPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// CDO 단계에서 기본 메시 바인딩. BP에서 슬롯이 비워져도 폴백 동작 보장
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPreviewMeshFinder(
		TEXT("/Game/3_Effects/PathPreview/SM_PreviewPoint.SM_PreviewPoint"));
	if (DefaultPreviewMeshFinder.Succeeded())
	{
		PreviewMesh = DefaultPreviewMeshFinder.Object;
	}
}

void UPRProjectileTrajectoryPreviewComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}
	
	GetWeaponManager();

	// Lazy 생성. 첫 호출 시 owner 액터에 ISMC을 동적 생성/등록.
	// 런타임 SceneComponent 생성 표준 패턴: NewObject 후 Mobility 설정과 SetupAttachment를 RegisterComponent 이전에 수행
	if (!IsValid(TrajectoryISMC))
	{
		TrajectoryISMC = NewObject<UInstancedStaticMeshComponent>(Owner);
		TrajectoryISMC->SetMobility(EComponentMobility::Movable);
		if (USceneComponent* OwnerRoot = Owner->GetRootComponent())
		{
			TrajectoryISMC->SetupAttachment(OwnerRoot);
		}
		TrajectoryISMC->SetStaticMesh(PreviewMesh);
		TrajectoryISMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TrajectoryISMC->SetGenerateOverlapEvents(false);
		TrajectoryISMC->SetCastShadow(false);
		// 머티리얼이 PerInstanceCustomData를 샘플링하여 일반/착탄 색상 분기 가능하도록 1채널 확보
		TrajectoryISMC->SetNumCustomDataFloats(1);
		TrajectoryISMC->RegisterComponent();
	}
}

void UPRProjectileTrajectoryPreviewComponent::SetPreviewEnabled(bool bInEnabled)
{
	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("SetPreviewEnabled 호출. Owner=%s, bOldEnabled=%d, bNewEnabled=%d, bIsShowing=%d, WeaponActor=%s, CachedWeaponManager=%s"),
		*GetNameSafe(GetOwner()),
		bIsEnabled,
		bInEnabled,
		bIsShowing,
		*GetNameSafe(WeaponActor.Get()),
		*GetNameSafe(CachedWeaponManager.Get()));

	bIsEnabled = bInEnabled;
}

/*~ Public API ~*/

void UPRProjectileTrajectoryPreviewComponent::SetFireParams(const FPRProjectilePreviewParams& InParams)
{
	FireParams = InParams;

	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("SetFireParams 호출. Owner=%s, InitialSpeed=%.2f, GravityScale=%.2f, CollisionRadius=%.2f, MaxSimTime=%.2f, SimFrequency=%.2f, SampleSpacing=%.2f, MaxSamplePoints=%d"),
		*GetNameSafe(GetOwner()),
		FireParams.InitialSpeed,
		FireParams.GravityScale,
		FireParams.CollisionRadius,
		FireParams.MaxSimTime,
		FireParams.SimFrequency,
		FireParams.SampleSpacing,
		FireParams.MaxSamplePoints);
}

void UPRProjectileTrajectoryPreviewComponent::SetWeaponActor(APRWeaponActor* InWeaponActor)
{
	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("SetWeaponActor 호출. Owner=%s, OldWeapon=%s, NewWeapon=%s, bIsShowing=%d, bIsEnabled=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(WeaponActor.Get()),
		*GetNameSafe(InWeaponActor),
		bIsShowing,
		bIsEnabled);

	WeaponActor = InWeaponActor;

	// 댕글링 방지: 무효 WeaponActor 주입 시 표시 강제 OFF
	if (!IsValid(InWeaponActor) && bIsShowing)
	{
		Hide();
	}
}

void UPRProjectileTrajectoryPreviewComponent::Show()
{
	GetWeaponManager();

	if (bIsShowing)
	{
		UE_LOG(LogPRPathPreviewComponent, Log,
			TEXT("Show 생략. 이미 표시 중, Owner=%s, bIsEnabled=%d, WeaponActor=%s, CachedWeaponManager=%s"),
			*GetNameSafe(GetOwner()),
			bIsEnabled,
			*GetNameSafe(WeaponActor.Get()),
			*GetNameSafe(CachedWeaponManager.Get()));
		return;
	}

	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("Show 호출. Owner=%s, bIsEnabled=%d, WeaponActor=%s, CachedWeaponManager=%s, TrajectoryISMC=%s"),
		*GetNameSafe(GetOwner()),
		bIsEnabled,
		*GetNameSafe(WeaponActor.Get()),
		*GetNameSafe(CachedWeaponManager.Get()),
		*GetNameSafe(TrajectoryISMC));

	bIsShowing = true;
	SetComponentTickEnabled(true);
}

void UPRProjectileTrajectoryPreviewComponent::Hide()
{
	if (!bIsShowing)
	{
		UE_LOG(LogPRPathPreviewComponent, Log,
			TEXT("Hide 생략. 이미 숨김 상태, Owner=%s, bIsEnabled=%d, WeaponActor=%s"),
			*GetNameSafe(GetOwner()),
			bIsEnabled,
			*GetNameSafe(WeaponActor.Get()));
		return;
	}

	UE_LOG(LogPRPathPreviewComponent, Log,
		TEXT("Hide 호출. Owner=%s, bIsEnabled=%d, WeaponActor=%s, InstanceCount=%d"),
		*GetNameSafe(GetOwner()),
		bIsEnabled,
		*GetNameSafe(WeaponActor.Get()),
		IsValid(TrajectoryISMC) ? TrajectoryISMC->GetInstanceCount() : 0);

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
	DrawTrajectoryISMC(SamplePoints);
}

void UPRProjectileTrajectoryPreviewComponent::ClearTrajectory()
{
	ClearTrajectoryISMC();
}

void UPRProjectileTrajectoryPreviewComponent::DrawTrajectoryISMC(const TArray<FVector>& SamplePoints)
{
	if (!IsValid(PreviewMesh))
	{
		return;
	}

	if (!IsValid(TrajectoryISMC))
	{
		return;
	}

	// 기존 인스턴스의 Transform만 갱신하고, 수량 차이만 Add/Remove로 보정
	const int32 NewCount = SamplePoints.Num();
	const int32 OldCount = TrajectoryISMC->GetInstanceCount();

	const bool bImpactDetected = LastResult.EndReason == EPRPreviewEndReason::HitBlocking;
	const int32 LastIndex = NewCount - 1;
	const FVector InstanceScale(PreviewMeshScale);

	// 기존 인스턴스 Transform 갱신 (공통 범위)
	const int32 UpdateCount = FMath::Min(OldCount, NewCount);
	for (int32 i = 0; i < UpdateCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		TrajectoryISMC->UpdateInstanceTransform(i, InstanceTransform, /*bWorldSpace=*/true,
		                                        /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryISMC->SetCustomDataValue(i, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 부족한 만큼 추가
	for (int32 i = OldCount; i < NewCount; ++i)
	{
		const FTransform InstanceTransform(FQuat::Identity, SamplePoints[i], InstanceScale);
		const int32 InstanceIndex = TrajectoryISMC->AddInstance(InstanceTransform, /*bWorldSpace=*/true);

		const float CustomValue = (bImpactDetected && i == LastIndex) ? 1.f : 0.f;
		TrajectoryISMC->SetCustomDataValue(InstanceIndex, 0, CustomValue, /*bMarkRenderStateDirty=*/false);
	}

	// 초과분 제거 (뒤에서부터)
	for (int32 i = OldCount - 1; i >= NewCount; --i)
	{
		TrajectoryISMC->RemoveInstance(i);
	}

	// 모든 인스턴스 갱신 완료 후 렌더 스테이트 1회 갱신
	TrajectoryISMC->MarkRenderStateDirty();
}

void UPRProjectileTrajectoryPreviewComponent::ClearTrajectoryISMC()
{
	if (IsValid(TrajectoryISMC))
	{
		TrajectoryISMC->ClearInstances();
	}
}

void UPRProjectileTrajectoryPreviewComponent::OnUnregister()
{
	if (IsValid(TrajectoryISMC))
	{
		TrajectoryISMC->DestroyComponent();
		TrajectoryISMC = nullptr;
	}
	Super::OnUnregister();
}

/*~ Tick ~*/

void UPRProjectileTrajectoryPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UPRWeaponManagerComponent* WeaponManager = GetWeaponManager())
	{
		WeaponActor = WeaponManager->GetActiveWeaponActor();	
	}
	
	// WeaponActor 유효성 재확인. 무기 교체 등으로 사라졌을 수 있음
	if (!WeaponActor.IsValid() || !bIsEnabled)
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose,
			TEXT("Tick 경로 생성 생략. Owner=%s, bIsEnabled=%d, bIsShowing=%d, WeaponActor=%s, CachedWeaponManager=%s"),
			*GetNameSafe(GetOwner()),
			bIsEnabled,
			bIsShowing,
			*GetNameSafe(WeaponActor.Get()),
			*GetNameSafe(CachedWeaponManager.Get()));
		return;
	}

	RebuildPath();
	DrawTrajectory(LastResult.SamplePoints);

	UE_LOG(LogPRPathPreviewComponent, Verbose,
		TEXT("Tick 경로 갱신 완료. Owner=%s, WeaponActor=%s, SamplePoints=%d, EndReason=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(WeaponActor.Get()),
		LastResult.SamplePoints.Num(),
		static_cast<int32>(LastResult.EndReason));
}

/*~ Internal ~*/

UPRWeaponManagerComponent* UPRProjectileTrajectoryPreviewComponent::GetWeaponManager()
{
	if (CachedWeaponManager.IsValid())
	{
		return CachedWeaponManager.Get();
	}

	APRPlayerCharacter* Player = Cast<APRPlayerCharacter>(GetOwner());
	if (!IsValid(Player))
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose, TEXT("WeaponManager 캐시 실패. Owner가 PlayerCharacter 아님, Owner=%s"),
			*GetNameSafe(GetOwner()));
		return nullptr;
	}

	CachedWeaponManager = Player->GetWeaponManager();
	if (CachedWeaponManager.IsValid())
	{
		UE_LOG(LogPRPathPreviewComponent, Log, TEXT("WeaponManager 캐시 완료. Owner=%s, WeaponManager=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CachedWeaponManager.Get()));
	}
	else
	{
		UE_LOG(LogPRPathPreviewComponent, Verbose, TEXT("WeaponManager 캐시 대기. Owner=%s"),
			*GetNameSafe(GetOwner()));
	}

	return CachedWeaponManager.Get();
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

	const FTransform LaunchTransform = UPRGameplayStatics::ResolveProjectileLaunchTransform(
		OwnerPawn, StartLocation, AimTraceDistance, FireParams.TraceChannel, IgnoredActors);
	const FVector LaunchVelocity = LaunchTransform.GetRotation().GetForwardVector() * FireParams.InitialSpeed;

	FPredictProjectilePathParams PathParams(FireParams.CollisionRadius, StartLocation, LaunchVelocity,
	                                        FireParams.MaxSimTime);
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
