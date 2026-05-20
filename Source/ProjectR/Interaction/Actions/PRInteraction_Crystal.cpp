// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRInteraction_Crystal.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "ProjectR/PRGameplayTags.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/Game/PRGameInstance.h"
#include "ProjectR/Game/PRGameStateBase.h"
#include "ProjectR/Interaction/PRInteractorComponent.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/Utils/PRGameplayStatics.h"

UPRInteraction_Crystal::UPRInteraction_Crystal()
{
	InteractionType = EPRInteractionType::Sustained;
	bRequiresRange = true;
}

void UPRInteraction_Crystal::Execute_Implementation(AActor* Interactor)
{
	Super::Execute_Implementation(Interactor);

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!IsValid(World) || World->GetTimerManager().IsTimerActive(TravelCheckTimerHandle))
	{
		return;
	}
	
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Crystal_Start);
	}

	if (TravelCheckDelay <= 0.0f)
	{
		CheckTravelCondition();
		return;
	}

	World->GetTimerManager().SetTimer(
		TravelCheckTimerHandle,
		this,
		&UPRInteraction_Crystal::CheckTravelCondition,
		TravelCheckDelay,
		false);
}

void UPRInteraction_Crystal::EndInteraction_Implementation(AActor* Interactor, bool bCanceled)
{
	Super::EndInteraction_Implementation(Interactor, bCanceled);
	
	if (UPRAbilitySystemComponent* ASC = Cast<UPRAbilitySystemComponent>(UPRGameplayStatics::GetAbilitySystemComponent(Interactor)))
	{
		if (bCanceled)
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(PRGameplayTags::Ability_Player_Crystal);
			ASC->CancelAbilities(&CancelTags);
		}
		else
		{
			ASC->MulticastTriggerEvent(PRGameplayTags::Event_Ability_Crystal_End);
		}
	}
	
	GetWorld()->GetTimerManager().ClearTimer(TravelCheckTimerHandle);
}

void UPRInteraction_Crystal::StartTravel()
{
	// TODO: 현재는 TargetMap을 Property에서 지정하지만, 추후 이동장소 선택 UI를 띄움
	if (!TargetMap.IsNull())
	{
		if (UPRGameInstance* GameInstance = GetWorld()->GetGameInstance<UPRGameInstance>())
		{
			GameInstance->ServerTravelToMap(TargetMap, false);
		}
	}
}

void UPRInteraction_Crystal::CheckTravelCondition()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
	{
		return;
	}

	const int32 FightCapablePlayerCount = CountFightCapablePlayers();
	if (FightCapablePlayerCount > 0 && CountInteractingPlayers() == FightCapablePlayerCount)
	{
		StartTravel();
	}
}

int32 UPRInteraction_Crystal::CountInteractingPlayers() const
{
	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return 0;
	}

	int32 InteractingPlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		AController* Controller = Cast<AController>(PRPlayerState->GetOwner());
		if (!IsValid(Controller) && IsValid(PRPlayerState->GetPawn()))
		{
			Controller = PRPlayerState->GetPawn()->GetController();
		}

		const UPRInteractorComponent* InteractorComponent = IsValid(Controller)
			? Controller->FindComponentByClass<UPRInteractorComponent>()
			: nullptr;
		if (IsValid(InteractorComponent) && InteractorComponent->GetActiveAction() == this)
		{
			++InteractingPlayerCount;
		}
	}

	return InteractingPlayerCount;
}

int32 UPRInteraction_Crystal::CountFightCapablePlayers() const
{
	UWorld* World = GetWorld();
	const APRGameStateBase* GameState = IsValid(World) ? World->GetGameState<APRGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return 0;
	}

	int32 FightCapablePlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant() || PRPlayerState->IsOutOfFight())
		{
			continue;
		}

		++FightCapablePlayerCount;
	}

	return FightCapablePlayerCount;
}
