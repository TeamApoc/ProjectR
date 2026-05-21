// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGameplayStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/Character/PRPlayerCharacter.h"
#include "ProjectR/Combat/PRCombatGameplayTags.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Weapon/Components/PRWeaponManagerComponent.h"

void UPRGameplayStatics::GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 현재 액터의 모든 MeshComponent 수집
	Actor->GetComponents<UMeshComponent>(OutMeshes, /*bIncludeFromChildActors=*/false);

	// ChildActorComponent를 통한 자식 액터 재귀 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	Actor->GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* ChildActorComp : ChildActorComps)
	{
		if (!IsValid(ChildActorComp))
		{
			continue;
		}

		TArray<UMeshComponent*> ChildMeshes;
		GetAllMeshComponents(ChildActorComp->GetChildActor(), ChildMeshes);
		OutMeshes.Append(ChildMeshes);
	}

	// 부착된 액터 재귀 탐색
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, /*bResetArray=*/true, /*bRecursivelyIncludeAttachedActors=*/false);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		GetAllMeshComponents(AttachedActor, AttachedMeshes);
		OutMeshes.Append(AttachedMeshes);
	}
}

UPRInventoryComponent* UPRGameplayStatics::GetInventoryComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetInventoryComponent();
		}
	}
	
	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetInventoryComponent();
	}
	
	return nullptr;
}

UPRWeaponManagerComponent* UPRGameplayStatics::GetWeaponManagerComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerCharacter* PlayerCharacter = Cast<APRPlayerCharacter>(AsPawn))
		{
			return PlayerCharacter->GetWeaponManager();
		}
	}
	
	return nullptr;
}

UAbilitySystemComponent* UPRGameplayStatics::GetAbilitySystemComponent(AActor* Actor)
{
	if (AController* AsController = Cast<AController>(Actor))
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AsController->GetPawn());
	}

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
}

void UPRGameplayStatics::GrantAmmo(AActor* TargetActor, TSubclassOf<UGameplayEffect> AmmoEffect,
	float AmmoAmount)
{
	if (!IsValid(TargetActor) || !IsValid(AmmoEffect) || !TargetActor->HasAuthority() || AmmoAmount <= 0.f)
	{
		return;
	}
	
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent(TargetActor);
	
	const UPRAttributeSet_Weapon* WeaponSet = TargetASC->GetSet<UPRAttributeSet_Weapon>();
	if (!IsValid(WeaponSet))
	{
		return;
	}

	// SetByCaller로 탄약량을 GE Spec에 전달
	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(AmmoEffect, 1.f, Context);
	SpecHandle.Data->SetSetByCallerMagnitude(PRCombatGameplayTags::SetByCaller_AmmoMagnitude, AmmoAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

UPRGameInstance* UPRGameplayStatics::GetPRGameInstance(const UObject* WorldContext)
{
	if (IsValid(WorldContext))
	{
		if (auto World = WorldContext->GetWorld())
		{
			return World->GetGameInstance<UPRGameInstance>();
		}
	}
	return nullptr;
}


bool UPRGameplayStatics::GetPawnViewpoint(const APawn* Pawn, FVector& OutLocation, FRotator& OutRotation)
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	if (!IsValid(Pawn))
	{
		return false;
	}

	// TPS 숄더뷰: SpringArm/카메라 오프셋이 반영된 실제 카메라 위치/회전 사용
	const APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (IsValid(PC) && IsValid(PC->PlayerCameraManager))
	{
		OutLocation = PC->PlayerCameraManager->GetCameraLocation();
		OutRotation = PC->PlayerCameraManager->GetCameraRotation();
		return true;
	}

	// 폴백: 컨트롤러 또는 카메라 매니저 부재 시 액터 눈높이 기준
	Pawn->GetActorEyesViewPoint(OutLocation, OutRotation);
	return true;
}

FVector UPRGameplayStatics::ResolveCameraAimPoint(const APawn* Pawn, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors)
{
	FVector CamLocation;
	FRotator CamRotation;
	if (!GetPawnViewpoint(Pawn, CamLocation, CamRotation))
	{
		return FVector::ZeroVector;
	}

	const FVector CamEnd = CamLocation + CamRotation.Vector() * TraceDistance;

	UWorld* World = IsValid(Pawn) ? Pawn->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return CamEnd;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PRCameraAimTrace), false);
	Params.AddIgnoredActors(IgnoredActors);
	Params.AddIgnoredActor(Pawn);

	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, CamLocation, CamEnd, TraceChannel, Params);

	return AimHit.bBlockingHit ? AimHit.ImpactPoint : CamEnd;
}

FTransform UPRGameplayStatics::ResolveProjectileLaunchTransform(const APawn* Pawn, const FVector& MuzzleLocation, float TraceDistance, ECollisionChannel TraceChannel, const TArray<AActor*>& IgnoredActors)
{
	FVector CamLocation;
	FRotator CamRotation;
	if (!GetPawnViewpoint(Pawn, CamLocation, CamRotation))
	{
		return FTransform(FQuat::Identity, MuzzleLocation);
	}

	const FVector AimPoint = ResolveCameraAimPoint(Pawn, TraceDistance, TraceChannel, IgnoredActors);

	// 머즐에서 조준점으로 향하는 방향. 거리가 너무 짧으면(조준점이 머즐 바로 앞/뒤 등) 카메라 정면으로 폴백
	FVector LaunchDir = AimPoint - MuzzleLocation;
	if (!LaunchDir.Normalize(KINDA_SMALL_NUMBER))
	{
		LaunchDir = CamRotation.Vector();
	}

	return FTransform(LaunchDir.Rotation(), MuzzleLocation);
}
