// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Fire.h"

#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/System/PREventManagerSubsystem.h"
#include "ProjectR/System/PREventTypes.h"
#include "ProjectR/UI/Crosshair/PRCrosshairTypes.h"
#include "ProjectR/Weapon/Actors/PRWeaponActor.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

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
}

void UPRGA_Fire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
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

	World->LineTraceSingleByChannel(Hit, MuzzleLoc, AimPoint, FireTraceChannel.GetValue(), Params);

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

void UPRGA_Fire::FireOneShot()
{
	const FGameplayAbilityActorInfo& Info = GetActorInfo();

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

	// UI/카메라 알림: 반동은 매 발사 시, 적중 신호는 적중 시에만 발송
	// 현재 활성 슬롯의 WeaponData를 가져오고, 그 안에 들어 있는 RecoilProfile을 그대로 사용
	if (UWorld* World = GetWorld())
	{
		if (UPREventManagerSubsystem* EventMgr = World->GetSubsystem<UPREventManagerSubsystem>())
		{
			FPRRecoilEventPayload RecoilPayload;
			if (APRPlayerCharacter* PlayerCharacter = GetPRCharacter<APRPlayerCharacter>())        
			{                                                                                      
				if (UPRWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManager())
				{                                                                                  
					const FPRActiveWeaponSlot& ActiveSlot = WeaponManager->GetActiveSlot();        
					if (IsValid(ActiveSlot.WeaponData))                                            
					{                                                                              
						RecoilPayload.RecoilProfile = ActiveSlot.WeaponData->RecoilProfile;        
					}                                                                              
				}                                                                                  
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
	// 실제 데미지 처리는 추후 GE 기반으로 구현. 현 단계는 로그만 남긴다
	const AActor* HitActor = Payload.ClientHitResult.IsValid() ? Payload.ClientHitResult->GetActor() : nullptr;
	UE_LOG(LogFire, Warning, TEXT("[ApplyDamage] ShotID=%u, Target=%s"),
		Payload.ShotID, IsValid(HitActor) ? *HitActor->GetName() : TEXT("None"));
}
