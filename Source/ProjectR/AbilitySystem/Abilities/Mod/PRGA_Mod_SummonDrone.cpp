// Copyright ProjectR. All Rights Reserved.

#include "PRGA_Mod_SummonDrone.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/Drone/PRSupportDroneActor.h"
#include "ProjectR/Drone/PRSupportDroneDataAsset.h"
#include "ProjectR/PRGameplayTags.h"

UPRGA_Mod_SummonDrone::UPRGA_Mod_SummonDrone()
{
	PlayerAbilityType = EPRPlayerAbilityType::Mod;
	InputTag = PRGameplayTags::Input_Ability_Mod;
}

bool UPRGA_Mod_SummonDrone::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!IsValid(DroneClass))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(PRGameplayTags::Fail_Invalid);
		}
		return false;
	}

	return true;
}

/*~ UPRGA_Mod_HasDuration Interface ~*/

void UPRGA_Mod_SummonDrone::OnDurationStarted_Implementation()
{
	Super::OnDurationStarted_Implementation();

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!IsValid(DroneClass) || !IsValid(SpawnSupportDrone(GetCurrentActorInfo())))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

/*~ UPRGA_Mod Interface ~*/

void UPRGA_Mod_SummonDrone::CleanupRuntimeOnAbilityRemoved(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 활성 드론 정리
	DestroyActiveDrone();
	Super::CleanupRuntimeOnAbilityRemoved(ActorInfo, Spec);
}

/*~ 소환 ~*/

APRSupportDroneActor* UPRGA_Mod_SummonDrone::SpawnSupportDrone(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ActorInfo == nullptr || !IsValid(DroneClass))
	{
		return nullptr;
	}

	APawn* PlayerPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!IsValid(PlayerPawn))
	{
		return nullptr;
	}

	UWorld* World = PlayerPawn->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	if (bReplaceExistingDrone)
	{
		DestroyActiveDrone();
	}
	else if (IsValid(ActiveDrone))
	{
		return ActiveDrone;
	}

	const FVector SpawnLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorRotation().RotateVector(SpawnOffset);
	const FRotator SpawnRotation = PlayerPawn->GetActorRotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerPawn;
	SpawnParameters.Instigator = PlayerPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APRSupportDroneActor* SpawnedDrone = World->SpawnActor<APRSupportDroneActor>(
		DroneClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
	if (!IsValid(SpawnedDrone))
	{
		return nullptr;
	}

	SpawnedDrone->InitializeDrone(PlayerPawn, DroneData);
	if (ModDuration > 0.0f)
	{
		// 모드 지속시간 기준 수명
		SpawnedDrone->SetLifeSpan(ModDuration);
	}
	ActiveDrone = SpawnedDrone;
	return SpawnedDrone;
}

void UPRGA_Mod_SummonDrone::DestroyActiveDrone()
{
	if (!IsValid(ActiveDrone))
	{
		ActiveDrone = nullptr;
		return;
	}

	ActiveDrone->Destroy();
	ActiveDrone = nullptr;
}
