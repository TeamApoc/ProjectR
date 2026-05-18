// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Fire.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameplayEffect.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/AbilitySystem/Data/PRAbilitySystemRegistry.h"
#include "ProjectR/AbilitySystem/Tasks/PRAT_SpawnPredictedProjectile.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Projectile/PRProjectileBase.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "ProjectR/Utils/PRGameplayStatics.h"
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

	// GetCooldownTags가 반환할 컨테이너 1회 초기화
	CooldownTagsContainer.AddTag(PRGameplayTags::Cooldown_Ability_Fire);
}

void UPRGA_Fire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
	{
		if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
		{
			CurrentWeapon = WeaponManager->GetActiveWeaponActor();
		}
	}
	
	// 발사 간격 결정. CommitAbilityCooldown 이전에 캐싱해야 ApplyCooldown SetByCaller 주입에 반영
	if (bOverrideFireInterval)
	{
		CachedFireInterval = FireIntervalOverride;
	}
	else if (const UPRWeaponDataAsset* WeaponData = GetActiveWeaponData())
	{
		CachedFireInterval = WeaponData->FireInterval;
	}
	else
	{
		CachedFireInterval = FireIntervalOverride;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	ResetConsecutiveShots();
	NextShotId = 0;
}

void UPRGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	NextShotId = 0;
	ResetConsecutiveShots();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPRGA_Fire::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 무기 교체시 쿨다운 효과 제거
	if (UPRGA_Fire* InstancedAbility = Cast<UPRGA_Fire>(Spec.GetPrimaryInstance()))
	{
		if (ActorInfo->AbilitySystemComponent.IsValid() && InstancedAbility->CooldownHandle.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(InstancedAbility->CooldownHandle);
		}
	}
	
	Super::OnRemoveAbility(ActorInfo, Spec);
}

/*~ 쿨다운 오버라이드 ~*/

UGameplayEffect* UPRGA_Fire::GetCooldownGameplayEffect() const
{
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->CooldownGE_Fire))
	{
		return nullptr;
	}
	return Registry->CooldownGE_Fire.GetDefaultObject();
}

const FGameplayTagContainer* UPRGA_Fire::GetCooldownTags() const
{
	return &CooldownTagsContainer;
}

void UPRGA_Fire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (CooldownGE == nullptr)
	{
		return;
	}
	
	if (UPRGA_Fire* InstancedAbility = GetAbilityInstance<UPRGA_Fire>(Handle, ActorInfo))
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass());
		if (!SpecHandle.IsValid())
		{
			return;
		}
				
		UE_LOG(LogTemp,Warning,TEXT("Ability Instance Name: %s "),*GetNameSafe(InstancedAbility));
		// 무기 데이터에서 캐싱한 발사 간격을 GE Duration으로 주입
		SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_Cooldown, InstancedAbility->CachedFireInterval);
		InstancedAbility->CooldownHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}


FVector UPRGA_Fire::GetMuzzleLocation()
{
	// !!! 임시 코드 !!!
	if (!CurrentWeapon.IsValid())
	{
		if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())
		{
			if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
			{
				CurrentWeapon = WeaponManager->GetActiveWeaponActor();
			}
		}
	}
	
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

	UPRGameplayStatics::GetPawnViewpoint(Cast<APawn>(GetAvatarActorFromActorInfo()), View.Location, View.Rotation);
	return View;
}

FVector UPRGA_Fire::ResolveAimPoint(const FPRFireViewpoint& View, float InMaxTraceDistance) const
{
	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	TArray<AActor*> IgnoredActors;
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		IgnoredActors.Add(AvatarActor);
	}
	const FVector AimPoint = UPRGameplayStatics::ResolveCameraAimPoint(OwnerPawn, InMaxTraceDistance, CameraTraceChannel.GetValue(), IgnoredActors);

	// 디버그: 카메라 트레이스는 시안색 (참고용)
	if (bDrawCameraTrace)
	{
		if (UWorld* World = GetWorld())
		{
			DrawDebugLine(World, View.Location, AimPoint, FColor::Cyan, false, DebugDrawDuration, 0, 0.5f);
		}
	}

	return AimPoint;
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

void UPRGA_Fire::SendRecoilEvent()
{
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
			RecoilPayload.bIsAiming = GetActorInfo().AbilitySystemComponent.IsValid()
				&& GetActorInfo().AbilitySystemComponent->HasMatchingGameplayTag(PRGameplayTags::State_Aiming);
			RecoilPayload.Speed = RecoilPayload.RecoilProfile.RecoilRecoverySpeed;
			RecoilPayload.Strength = RecoilPayload.RecoilProfile.CrosshairSpreadIncrease;
			EventMgr->BroadcastTyped(PRGameplayTags::Event_Player_Recoil, RecoilPayload);
		}
	}
}

void UPRGA_Fire::FireHitScan()
{
	const FGameplayAbilityActorInfo& Info = GetActorInfo();
	
	// 클라이언트 예측 cost 적용 (호스트는 ServerConfirmShot에서 단일 auth commit 처리하므로 제외)
	// 예측 cost가 실패하면 발사 시도 자체를 차단하고 어빌리티 종료
	const AActor* AvatarActor = Info.AvatarActor.Get();
	const bool bHasAuthority = IsValid(AvatarActor) && AvatarActor->HasAuthority();
	if (!bHasAuthority)
	{
		if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
		{
			UE_LOG(LogFire, Verbose, TEXT("Client predicted cost failed (탄약 부족). 사격 차단."));
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
			return;
		}
	}
	
	// 몽타쥬 재생. 무기 메시 애니메이션은 몽타주 노티파이가 각 머신에서 로컬로 트리거한다
	if (UPRWeaponDataAsset* WeaponData = GetActiveWeaponData())
	{
		PlayWeaponMontage(WeaponData->ShootMontage, WeaponData->ShootMontagePlayRate);
	}
	
	// 로컬에서만 트레이스/리포트 수행. 시뮬레이트 프록시는 무시
	if (!Info.IsLocallyControlled())
	{
		return;
	}

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

	SendRecoilEvent();
	
	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
			{
				EventMgr->BroadcastEmpty(PRGameplayTags::Event_Player_HitShot);
			}
		}
	}
	

	// 권위가 있는 로컬(Standalone/ListenServer 호스트)은 RPC 없이 직접 확정
	if (bHasAuthority)
	{
		ServerConfirmShot(Payload);
	}
	// 권위가 없는 클라는 서버에 보고
	else
	{
		Server_ReportShot(Payload);
	}
}

void UPRGA_Fire::ResetConsecutiveShots()
{
	ConsecutiveShots = 0;
}

FTransform UPRGA_Fire::GetProjectileLaunchTransform()
{
	if (!GetActorInfo().IsLocallyControlled())
	{
		return FTransform();
	}

	APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	TArray<AActor*> IgnoredActors;
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		IgnoredActors.Add(AvatarActor);
	}
	return UPRGameplayStatics::ResolveProjectileLaunchTransform(OwnerPawn, GetMuzzleLocation(), MaxTraceDistance, FireTraceChannel.GetValue(), IgnoredActors);
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
	
	// 몽타쥬 재생. 무기 메시 애니메이션은 몽타주 노티파이가 각 머신에서 로컬로 트리거한다
	if (UPRWeaponDataAsset* WeaponData = GetActiveWeaponData())
	{
		PlayWeaponMontage(WeaponData->ShootMontage, WeaponData->ShootMontagePlayRate);
	}
	
	// 반동 이벤트 전송
	if (GetActorInfo().IsLocallyControlled())
	{
		SendRecoilEvent();
	}
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

	// GA의 CostGameplayEffectClass를 적용해 슬롯 탄창에서 발사 비용을 차감
	// CheckCost 실패 시 탄약 부족으로 간주해 데미지를 적용하지 않고 어빌리티 종료
	if (!CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogFire, Warning, TEXT("Cost commit failed (탄약 부족). ShotID: %u"), Payload.ShotID);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

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

	const FGameplayEffectSpecHandle SpecHandle = MakeWeaponEffectSpec(HitResult);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(TargetASC))
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

FGameplayEffectSpecHandle UPRGA_Fire::MakeWeaponEffectSpec(const FHitResult* HitResult) const
{
	const UPRAbilitySystemRegistry* Registry = UPRAssetManager::Get().GetAbilitySystemRegistry();
	if (!IsValid(Registry) || !IsValid(Registry->DamageGE_FromWeapon))
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		Registry->DamageGE_FromWeapon);

	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	// HitResult가 있으면 EffectContext에 포함시켜 ExecCalc에서 부위 판정에 활용한다
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		SpecHandle.Data->GetContext().AddHitResult(*HitResult, true);
	}

	return SpecHandle;
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

