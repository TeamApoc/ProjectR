// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Fire.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_SpawnPredictedProjectile.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"
#include "ProjectR/Weapon/Data/PRWeaponDataAsset.h"

DEFINE_LOG_CATEGORY(LogFire);

UPRGA_Fire::UPRGA_Fire()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 기본적으로 발사 어빌리티는 Aiming일 때만 활성화
	ActivationRequiredTags.AddTag(PRGameplayTags::State_Aiming);
}

void UPRGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ResetConsecutiveShots();

	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
	{
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			CurrentWeapon = WeaponManager->GetActiveWeaponActor();
		}
	}

	NextShotId = 0;
}

void UPRGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	NextShotId = 0;
	ResetConsecutiveShots();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


FVector UPRGA_Fire::GetMuzzleLocation() const
{
	// !!! 임시 코드 !!!
	if (CurrentWeapon.IsValid())
	{
		return CurrentWeapon->GetMuzzleTransform().GetLocation();
	}

	// 테스트용 Fallback
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		return AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 50.f;
	}

	return FVector::ZeroVector;
}

FPRFireViewpoint UPRGA_Fire::GetFireViewpoint() const
{
	FPRFireViewpoint View;

	if (!ensureMsgf(GetActorInfo().IsLocallyControlled(), TEXT("Viewpoint는 로컬에서만 유효.")))
	{
		return View;
	}

	// TPS 숄더뷰: SpringArm/카메라 오프셋이 반영된 실제 카메라 위치/회전을 사용
	// (Pawn::GetActorEyesViewPoint는 액터 BaseEyeHeight 기준이라 TPS 카메라와 다름)
	APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	if (IsValid(PC) && IsValid(PC->PlayerCameraManager))
	{
		View.Location = PC->PlayerCameraManager->GetCameraLocation();
		View.Rotation = PC->PlayerCameraManager->GetCameraRotation();
		return View;
	}

	// Fallback: 컨트롤러가 없을 때 액터 눈높이 기준
	if (APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		OwnerPawn->GetActorEyesViewPoint(View.Location, View.Rotation);
	}

	return View;
}

FVector UPRGA_Fire::ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const
{
	const FVector CamStart = View.Location;
	const FVector CamEnd = CamStart + View.Rotation.Vector() * InMaxTraceDistance;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return CamEnd;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRFireAimTrace), false);
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		Params.AddIgnoredActor(AvatarActor);
	}

	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, CamStart, CamEnd, FireTraceChannel.GetValue(), Params);

	// 디버그: 카메라 트레이스는 시안색 (참고용)
	if (bDrawCameraTrace)
	{
		const FVector AimDrawEnd = AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
		DrawDebugLine(World, CamStart, AimDrawEnd, FColor::Cyan, false, DebugDrawDuration, 0, 0.5f);
	}

	return AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
}

FHitResult UPRGA_Fire::PerformMuzzleTrace(const FVector& MuzzleLoc, const FVector& AimPoint) const
{
	FHitResult Hit;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return Hit;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRFireMuzzleTrace), false);
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		Params.AddIgnoredActor(AvatarActor);
	}
	
	FVector Dir = AimPoint - MuzzleLoc;
	Dir.Normalize();
	FVector EndPoint = AimPoint + Dir * 30.f; // AimPoint에서 바로 멈추면 히트 실패 가능 있으므로 약간의 여유분 추가

	World->LineTraceSingleByChannel(Hit, MuzzleLoc, EndPoint, FireTraceChannel.GetValue(), Params);

	// 디버그 라인: 실제 탄도(총구 -> 끝점). 빨강
	if (bDrawMuzzleTrace)
	{
		const FVector DrawEnd = Hit.bBlockingHit ? Hit.ImpactPoint : AimPoint;
		DrawDebugLine(World, MuzzleLoc, DrawEnd, FColor::Red, false, DebugDrawDuration, 0, 1.0f);
		if (Hit.bBlockingHit)
		{
			DrawDebugPoint(World, Hit.ImpactPoint, 8.0f, FColor::Yellow, false, DebugDrawDuration);
		}
	}

	return Hit;
}

void UPRGA_Fire::FireHitScan()
{
	const FGameplayAbilityActorInfo& Info = GetActorInfo();

	// 로컬에서만 트레이스/리포트 수행. 시뮬레이트 프록시는 무시
	if (!Info.IsLocallyControlled())
	{
		return;
	}

	// 몽타쥬 재생
	if (UPRWeaponDataAsset* WeaponData = GetActiveWeaponData())
	{
		PlayWeaponMontage(WeaponData->ShootMontage, WeaponData->ShootMontagePlayRate);
	}
	RequestCurrentWeaponShootAnimation();

	// 페이로드 구성
	FPRFireShotPayload Payload;
	Payload.ShotID = ++NextShotId;

	const FPRFireViewpoint View = GetFireViewpoint();
	const FVector MuzzleLoc = GetMuzzleLocation();
	Payload.ShotOrigin = FVector_NetQuantize(MuzzleLoc);

	// 1차: 카메라 트레이스로 조준 끝점 산출
	const FVector AimPoint = ResolveAimPoint(View, MaxTraceDistance);

	// 2차: 총구에서 조준점으로 실제 발사 트레이스
	const FHitResult Hit = PerformMuzzleTrace(MuzzleLoc, AimPoint);

	if (Hit.GetActor() != nullptr)
	{
		Payload.ClientHitResult = MakeShared<FHitResult>(Hit);
	}
	else
	{
		// 미스: 총구 기준 조준 끝점 방향을 보고한다 (숄더뷰 보정)
		const FVector MuzzleToAim = (AimPoint - MuzzleLoc).GetSafeNormal();
		Payload.ShotDirection = FVector_NetQuantize(MuzzleToAim);
	}

	Payload.ClientTimestamp = IsValid(GetWorld()) ? GetWorld()->GetTimeSeconds() : 0.f;

	UE_LOG(LogFire, Verbose, TEXT("[Local] FireOneShot. ShotID=%u, Hit=%s"),
		Payload.ShotID, Hit.bBlockingHit ? TEXT("true") : TEXT("false"));

	++ConsecutiveShots;

	// UI/카메라 알림: 반동은 매 발사 시, 적중 신호는 적중 시에만 발송
	// 현재 활성 슬롯의 WeaponData를 가져오고, 그 안에 들어 있는 RecoilProfile을 그대로 사용
	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			FPRRecoilEventPayload RecoilPayload;
			if (UPRWeaponDataAsset* WeaponData = GetActiveWeaponData())   
			{
				RecoilPayload.RecoilProfile = WeaponData->RecoilProfile;
			}                                                                                                                                                   
			RecoilPayload.ConsecutiveShots = ConsecutiveShots;
			RecoilPayload.bIsAiming = Info.AbilitySystemComponent.IsValid()
				&& Info.AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Aiming);
			RecoilPayload.Speed = RecoilPayload.RecoilProfile.RecoilRecoverySpeed;
			RecoilPayload.Strength = RecoilPayload.RecoilProfile.CrosshairSpreadIncrease;
			EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_Recoil, RecoilPayload);

			if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
			{
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_HitShot);
			}
		}
	}

	// 권위가 있는 로컬(Standalone/ListenServer 호스트)은 RPC 없이 직접 확정
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const bool bHasAuthority = IsValid(AvatarActor) && AvatarActor->HasAuthority();

	if (bHasAuthority)
	{
		ServerConfirmShot(Payload);
	}
	else
	{
		Server_ReportShot(Payload);
	}
}

void UPRGA_Fire::ResetConsecutiveShots()
{
	ConsecutiveShots = 0;
}

FTransform UPRGA_Fire::GetProjectileLaunchTransform() const
{
	if (!GetActorInfo().IsLocallyControlled())
	{
		return FTransform();
	}
	
	const FPRFireViewpoint View = GetFireViewpoint();
	const FVector MuzzleLoc = GetMuzzleLocation();

	// 1차 트레이스로 카메라가 가리키는 월드 조준점 산출 (숄더뷰 시차 보정)
	const FVector AimPoint = ResolveAimPoint(View, MaxTraceDistance);

	// 총구 -> 조준점 방향. 거리가 너무 짧으면(=조준점이 총구 바로 앞/뒤) 카메라 정면으로 폴백
	FVector LaunchDir = AimPoint - MuzzleLoc;
	if (!LaunchDir.Normalize(KINDA_SMALL_NUMBER))
	{
		LaunchDir = View.Rotation.Vector();
	}

	return FTransform(LaunchDir.Rotation(), MuzzleLoc);
}

void UPRGA_Fire::FireProjectile(TSubclassOf<APRProjectileBase> ProjectileClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	if (!IsValid(ProjectileClass))
	{
		UE_LOG(LogFire, Warning, TEXT("FireProjectile: 유효하지 않은 ProjectileClass 전달"));
		return;
	}

	UPRAT_SpawnPredictedProjectile* Task = UPRAT_SpawnPredictedProjectile::SpawnPredictedProjectile(
		this, ProjectileClass, SpawnLocation, SpawnRotation);
	if (!Task)
	{
		UE_LOG(LogFire, Warning, TEXT("FireProjectile: AbilityTask 생성 실패. Class=%s"), *GetNameSafe(ProjectileClass));
		return;
	}

	// AbilityTask 결과 콜백 바인딩. 결과 통지는 OnProjectileSpawnSuccess/Failed 핸들러로 일원화
	Task->OnSuccess.BindDynamic(this, &UPRGA_Fire::OnProjectileSpawnSuccess);
	Task->OnFailed.BindDynamic(this, &UPRGA_Fire::OnProjectileSpawnFailed);

	UE_LOG(LogFire, Verbose, TEXT("FireProjectile: 시작. Class=%s, Loc=%s, Rot=%s"),
		*GetNameSafe(ProjectileClass), *SpawnLocation.ToCompactString(), *SpawnRotation.ToCompactString());

	// ReadyForActivation 호출 시 AT::Activate가 실행되어 클라/서버 각자의 스폰 흐름이 진행됨
	Task->ReadyForActivation();
}

void UPRGA_Fire::OnProjectileSpawnSuccess(APRProjectileBase* SpawnedProjectile)
{
	if (!IsValid(SpawnedProjectile))
	{
		UE_LOG(LogFire, Warning, TEXT("OnProjectileSpawnSuccess: 무효 인스턴스 수신"));
		return;
	}

	UE_LOG(LogFire, Verbose, TEXT("OnProjectileSpawnSuccess: Id=%u, Actor=%s"),
		SpawnedProjectile->GetProjectileId(), *GetNameSafe(SpawnedProjectile));
	
	K2_OnProjectileSpawnSuccess(SpawnedProjectile);
}

void UPRGA_Fire::OnProjectileSpawnFailed(APRProjectileBase* SpawnedProjectile)
{
	UE_LOG(LogFire, Warning, TEXT("OnProjectileSpawnFailed: 투사체 스폰 실패 또는 예측 거부"));
	K2_OnProjectileSpawnFailed(SpawnedProjectile);
}


void UPRGA_Fire::Server_ReportShot_Implementation(FPRFireShotPayload Payload)
{
	if (!Payload.IsValidShotID())
	{
		UE_LOG(LogFire, Warning, TEXT("Server Received Invalid Shot. ShotID: %u"), Payload.ShotID);
		return;
	}

	UE_LOG(LogFire, Warning, TEXT("Server Received Shot. ShotID: %u"), Payload.ShotID);

	if (Payload.HasValidHitResult())
	{
		UE_LOG(LogFire, Warning, TEXT("Shot has HitTarget. ShotID: %u"), Payload.ShotID);
	}
	else
	{
		UE_LOG(LogFire, Warning, TEXT("Shot missed. ShotID: %u"), Payload.ShotID);
	}

	ServerConfirmShot(Payload);
}

void UPRGA_Fire::ServerConfirmShot(const FPRFireShotPayload& Payload)
{
	UE_LOG(LogFire, Warning, TEXT("Server Confirm Shot. ShotID: %u"), Payload.ShotID);

	// TODO: Cost GE 적용 (탄약 소모)
	// TODO: Tolerance 검증 + ShotId gap 감지

	if (Payload.HasValidHitResult())
	{
		ApplyDamageFromShot(Payload);
	}
}

void UPRGA_Fire::ApplyDamageFromShot(const FPRFireShotPayload& Payload)
{
	if (!Payload.ClientHitResult.IsValid())
	{
		return;
	}

	AActor* HitActor = Payload.ClientHitResult->GetActor();
	if (!IsValid(HitActor))
	{
		return;
	}

	ApplyDamage(HitActor, Payload.ClientHitResult.Get());
}

void UPRGA_Fire::ApplyDamage(AActor* TargetActor, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC))
	{
		return;
	}

	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromWeapon))
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		Registry->DamageGE_FromWeapon);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// HitResult가 있으면 EffectContext에 포함시켜 ExecCalc에서 부위 판정에 활용한다
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

UPRWeaponDataAsset* UPRGA_Fire::GetActiveWeaponData() const                                                            
{                                                                                                                      
	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())                                  
	{                                                                                                                
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())                      
		{                                                                                                        
			const FPRWeaponVisualInfo& CurrentWeaponVisualInfo = WeaponManager->GetCurrentWeaponVisualInfo();
			return CurrentWeaponVisualInfo.WeaponData;                                                       
		}                                                                                                        
	}                                                                                                                
                                                                                                                       
	return nullptr;                                                                                                  
}                                                                                                                      

void UPRGA_Fire::PlayWeaponMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!IsValid(Montage))
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->PlayMontage(
				this,
				CurrentActivationInfo,
				Montage,
				FMath::Max(PlayRate, UE_SMALL_NUMBER));
	}
}

void UPRGA_Fire::RequestCurrentWeaponShootAnimation() const
{
	if (CurrentWeapon.IsValid())
	{
		CurrentWeapon->RequestShootAnimation();
	}
}
