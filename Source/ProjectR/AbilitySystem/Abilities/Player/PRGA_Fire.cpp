// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGA_Fire.h"

#include "ProjectR/PRGameplayTags.h"

DEFINE_LOG_CATEGORY(LogFire);

UPRGA_Fire::UPRGA_Fire()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	// 기본적으로 발사 어빌리티는 Aiming일 때만 활성화
	ActivationRequiredTags.AddTag(PRGameplayTags::State_Aiming);
}

FVector UPRGA_Fire::GetMuzzleLocation() const
{
	// TODO: 실제 부착된 총구 소켓 로케이션 반환
	
	// !!! 임시 코드 !!!
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		// !!! 임시 코드 !!!
		return AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * 50.f;
	}
	
	return FVector();
}

FPRFireViewpoint UPRGA_Fire::GetFireViewpoint() const
{
	FPRFireViewpoint View;
	
	if (!ensureMsgf(GetActorInfo().IsLocallyControlled(), TEXT("Viewpoint는 로컬에서만 유효.")))
	{
		return View;
	}
	
	if (APawn* OwnerPawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		// 컨트롤러 기반의 카메라 위치와 회전값 획득
		OwnerPawn->GetActorEyesViewPoint(View.Location, View.Rotation);
	}
    
	return View;
}

FHitResult UPRGA_Fire::PerformFireTrace(const FPRFireViewpoint& View, float MaxTraceDistance) const
{
	return FHitResult();	
}

void UPRGA_Fire::Server_ReportShot_Implementation(FPRFireShotPayload Payload)
{
	if (!Payload.IsValidShotID())
	{
		UE_LOG(LogFire,Warning,TEXT("Server Received Invalid Shot. ShotID: %u"),Payload.ShotID);
		return;
		
	}
	
	UE_LOG(LogFire,Warning,TEXT("Server Received Shot. ShotID: %u"),Payload.ShotID);
	
	if (Payload.HasValidHitResult())
	{
		UE_LOG(LogFire,Warning,TEXT("Shot has HitTarget. ShotID: %u"),Payload.ShotID);
		ServerConfirmShot(Payload);	
	}
	else
	{
		UE_LOG(LogFire,Warning,TEXT("Shot missed. ShotID: %u"),Payload.ShotID);
	}
}

void UPRGA_Fire::FireOneShot()
{
	
}

void UPRGA_Fire::ServerConfirmShot(const FPRFireShotPayload& Payload)
{
	UE_LOG(LogFire,Warning,TEXT("Server Confirm Shot. ShotID: %u"),Payload.ShotID);
}
