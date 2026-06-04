// Copyright ProjectR. All Rights Reserved.

#include "PRItemDropManagerSubsystem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "ProjectR/AbilitySystem/AttributeSets/PRAttributeSet_Weapon.h"
#include "ProjectR/AbilitySystem/PRAbilitySystemComponent.h"
#include "ProjectR/ItemSystem/Components/PRInventoryComponent.h"
#include "ProjectR/ItemSystem/Data/PRAmmoDataAsset.h"
#include "ProjectR/ItemSystem/Data/PRItemDataAsset.h"
#include "ProjectR/ItemSystem/Items/PRItemInstance.h"
#include "ProjectR/Player/Components/PRCurrencyComponent.h"
#include "ProjectR/Player/Components/PRPlayerGrowthComponent.h"
#include "ProjectR/Player/PRPlayerController.h"
#include "ProjectR/Player/PRPlayerState.h"
#include "ProjectR/ProjectR.h"
#include "ProjectR/System/PRAssetManager.h"
#include "ProjectR/System/PRDeveloperSettings.h"
#include "ProjectR/World/Pickable/PRRewardPickupActor.h"

namespace
{
	constexpr float RewardPickupGroundTraceUpDistance = 200.0f;
	constexpr float RewardPickupGroundTraceDownDistance = 3000.0f;
	constexpr float RewardPickupSpawnHeight = 120.0f;
}

void UPRItemDropManagerSubsystem::HandleMonsterDied(const FPRMonsterDeathDropRequest& Request)
{
	UWorld* World = GetWorld();
	AActor* DeadMonster = Request.DeadMonster.Get();
	if (!IsValid(World) || !IsValid(DeadMonster) || !DeadMonster->HasAuthority())
	{
		return;
	}

	if (ProcessedDeadMonsters.Contains(DeadMonster))
	{
		return;
	}
	ProcessedDeadMonsters.Add(DeadMonster);

	if (Request.MonsterId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] MonsterId가 없어 드롭을 건너뜀. DeadMonster = %s"), *GetNameSafe(DeadMonster));
		return;
	}

	const FPRMonsterDropTableRow* DropRow = UPRAssetManager::Get().FindMonsterDropRow(Request.MonsterId);
	if (DropRow == nullptr)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Drop][Server] 드롭 Row 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	GrantExperienceReward(*DropRow, Request);

	for (const FPRDropRewardEntry& Entry : DropRow->Rewards)
	{
		FPRResolvedDropReward Reward;
		if (!ResolveReward(Entry, Reward))
		{
			continue;
		}

		CommitResolvedReward(Reward, Request);
	}
}

bool UPRItemDropManagerSubsystem::ClaimPickup(APRRewardPickupActor* PickupActor, AActor* Interactor)
{
	if (!IsValid(PickupActor) || !PickupActor->HasAuthority() || PickupActor->IsClaimed())
	{
		return false;
	}

	if (!PickupActor->CanBeClaimedBy(Interactor))
	{
		return false;
	}

	if (ClaimedPickups.Contains(PickupActor))
	{
		return false;
	}

	AController* InteractorController = ResolveInteractorController(Interactor);
	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(PickupActor->GetReward().DistributionRule, InteractorController, Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 픽업 지급 대상 없음. Pickup = %s"), *GetNameSafe(PickupActor));
		return false;
	}

	bool bGrantedAny = false;
	for (APRPlayerState* Recipient : Recipients)
	{
		bGrantedAny |= GrantRewardToPlayer(Recipient, PickupActor->GetReward());
	}

	if (!bGrantedAny)
	{
		return false;
	}

	ClaimedPickups.Add(PickupActor);
	PickupActor->MarkClaimed();
	PickupActor->Destroy();
	return true;
}

bool UPRItemDropManagerSubsystem::ResolveReward(const FPRDropRewardEntry& Entry, FPRResolvedDropReward& OutReward) const
{
	if (Entry.RewardType == EPRRewardType::None || Entry.DropChance <= 0.0f)
	{
		return false;
	}

	if (FMath::FRand() > Entry.DropChance)
	{
		return false;
	}

	OutReward.RewardType = Entry.RewardType;
	OutReward.DistributionRule = Entry.DistributionRule;
	OutReward.bSpawnPickup = Entry.bSpawnPickup;

	if (Entry.RewardType == EPRRewardType::Currency)
	{
		if (Entry.MinScrap <= 0 || Entry.MaxScrap < Entry.MinScrap)
		{
			return false;
		}

		OutReward.ScrapAmount = FMath::RandRange(Entry.MinScrap, Entry.MaxScrap);
		return OutReward.ScrapAmount > 0;
	}

	if (Entry.RewardType == EPRRewardType::Item || Entry.RewardType == EPRRewardType::Ammo)
	{
		if (!Entry.ItemAssetId.IsValid() || Entry.MinQuantity <= 0 || Entry.MaxQuantity < Entry.MinQuantity)
		{
			return false;
		}

		UPRItemDataAsset* ItemData = UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Entry.ItemAssetId);
		if (!IsValid(ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 아이템 데이터 조회 실패. ItemAssetId = %s"), *Entry.ItemAssetId.ToString());
			return false;
		}

		if (Entry.RewardType == EPRRewardType::Ammo && !IsValid(Cast<UPRAmmoDataAsset>(ItemData)))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 데이터 타입 불일치. ItemAssetId = %s"), *Entry.ItemAssetId.ToString());
			return false;
		}

		OutReward.ItemAssetId = Entry.ItemAssetId;
		OutReward.ItemData = ItemData;
		OutReward.Quantity = FMath::RandRange(Entry.MinQuantity, Entry.MaxQuantity);
		return OutReward.Quantity > 0;
	}

	return false;
}

void UPRItemDropManagerSubsystem::CommitResolvedReward(const FPRResolvedDropReward& Reward, const FPRMonsterDeathDropRequest& Request)
{
	if (Reward.bSpawnPickup)
	{
		SpawnRewardPickup(Reward, Request.DropLocation, Request.DeadMonster.Get());
		return;
	}

	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(Reward.DistributionRule, Request.KillerController.Get(), Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 즉시 지급 대상 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	for (APRPlayerState* Recipient : Recipients)
	{
		GrantRewardToPlayer(Recipient, Reward);
	}
}

void UPRItemDropManagerSubsystem::GrantExperienceReward(const FPRMonsterDropTableRow& DropRow, const FPRMonsterDeathDropRequest& Request) const
{
	if (DropRow.Experience <= 0)
	{
		return;
	}

	TArray<APRPlayerState*> Recipients;
	ResolveRecipients(DropRow.ExperienceDistributionRule, Request.KillerController.Get(), Recipients);
	if (Recipients.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 경험치 지급 대상 없음. MonsterId = %s"), *Request.MonsterId.ToString());
		return;
	}

	for (APRPlayerState* Recipient : Recipients)
	{
		if (!IsValid(Recipient))
		{
			continue;
		}

		UPRPlayerGrowthComponent* GrowthComponent = Recipient->GetGrowthComponent();
		if (!IsValid(GrowthComponent))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 경험치 지급 실패. GrowthComponent 없음. PlayerState = %s"), *GetNameSafe(Recipient));
			continue;
		}

		GrowthComponent->AddExperience(DropRow.Experience);
	}
}

APRRewardPickupActor* UPRItemDropManagerSubsystem::SpawnRewardPickup(const FPRResolvedDropReward& Reward, const FVector& DropLocation, const AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	TSubclassOf<APRRewardPickupActor> PickupClass = APRRewardPickupActor::StaticClass();
	const UPRDeveloperSettings* Settings = GetDefault<UPRDeveloperSettings>();
	if (Settings != nullptr && !Settings->RewardPickupActorClass.IsNull())
	{
		PickupClass = Settings->RewardPickupActorClass.LoadSynchronous();
	}

	if (!IsValid(PickupClass.Get()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] RewardPickupActorClass 로드 실패"));
		return nullptr;
	}

	const FVector SpawnLocation = ResolveRewardPickupSpawnLocation(DropLocation, IgnoredActor);
	APRRewardPickupActor* PickupActor = World->SpawnActor<APRRewardPickupActor>(
		PickupClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);
	if (!IsValid(PickupActor))
	{
		return nullptr;
	}

	PickupActor->InitializeReward(Reward);
	return PickupActor;
}

FVector UPRItemDropManagerSubsystem::ResolveRewardPickupSpawnLocation(const FVector& DropLocation, const AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return DropLocation;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PRRewardPickupGroundTrace), false);
	if (IsValid(IgnoredActor))
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	const FVector TraceStart = DropLocation + FVector::UpVector * RewardPickupGroundTraceUpDistance;
	const FVector TraceEnd = DropLocation - FVector::UpVector * RewardPickupGroundTraceDownDistance;

	FHitResult HitResult;
	const bool bHitGround = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		PRCollisionChannels::ECC_Ground,
		QueryParams);

	const FVector GroundLocation = bHitGround ? HitResult.ImpactPoint : DropLocation;
	return GroundLocation + FVector::UpVector * RewardPickupSpawnHeight;
}

void UPRItemDropManagerSubsystem::ResolveRecipients(EPRRewardDistributionRule DistributionRule, AController* PersonalController, TArray<APRPlayerState*>& OutRecipients) const
{
	OutRecipients.Reset();

	if (DistributionRule == EPRRewardDistributionRule::Personal)
	{
		APRPlayerState* PlayerState = IsValid(PersonalController) ? PersonalController->GetPlayerState<APRPlayerState>() : nullptr;
		if (IsValid(PlayerState))
		{
			OutRecipients.Add(PlayerState);
		}
		return;
	}

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState<AGameStateBase>() : nullptr;
	if (!IsValid(GameState))
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		APRPlayerState* PRPlayerState = Cast<APRPlayerState>(PlayerState);
		if (!IsValid(PRPlayerState) || !PRPlayerState->IsCombatParticipant())
		{
			continue;
		}

		OutRecipients.Add(PRPlayerState);
	}
}

AController* UPRItemDropManagerSubsystem::ResolveInteractorController(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller;
	}

	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn->GetController();
	}

	return nullptr;
}

bool UPRItemDropManagerSubsystem::GrantRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	if (!IsValid(PlayerState) || !PlayerState->HasAuthority())
	{
		return false;
	}

	if (Reward.RewardType == EPRRewardType::Currency)
	{
		UPRCurrencyComponent* CurrencyComponent = PlayerState->GetCurrencyComponent();
		if (!IsValid(CurrencyComponent))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Drop][Server] 고철 지급 실패. PlayerState = %s | Reason = InvalidCurrencyComponent | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				Reward.ScrapAmount);
			return false;
		}

		const bool bGranted = CurrencyComponent->AddScrap(Reward.ScrapAmount);
		if (bGranted)
		{
			NotifyPickupRewardGranted(PlayerState, Reward);
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[Drop][Server] 고철 지급 완료. PlayerState = %s | DistributionRule = %d | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				static_cast<uint8>(Reward.DistributionRule),
				Reward.ScrapAmount);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Drop][Server] 고철 지급 실패. PlayerState = %s | DistributionRule = %d | ScrapAmount = %d"),
				*GetNameSafe(PlayerState),
				static_cast<uint8>(Reward.DistributionRule),
				Reward.ScrapAmount);
		}
		return bGranted;
	}

	if (Reward.RewardType == EPRRewardType::Ammo)
	{
		return GrantAmmoRewardToPlayer(PlayerState, Reward);
	}

	if (Reward.RewardType == EPRRewardType::Item)
	{
		UPRInventoryComponent* InventoryComponent = PlayerState->GetInventoryComponent();
		UPRItemInstance* AddedItem = IsValid(InventoryComponent) && IsValid(Reward.ItemData)
			? InventoryComponent->AddItem(Reward.ItemData, Reward.Quantity)
			: nullptr;
		const bool bGranted = IsValid(AddedItem);
		if (bGranted)
		{
			NotifyPickupRewardGranted(PlayerState, Reward);
		}
		return bGranted;
	}

	return false;
}

bool UPRItemDropManagerSubsystem::GrantAmmoRewardToPlayer(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	if (!IsValid(PlayerState) || !PlayerState->HasAuthority() || Reward.Quantity <= 0)
	{
		return false;
	}

	UPRAmmoDataAsset* AmmoData = Cast<UPRAmmoDataAsset>(Reward.ItemData);
	if (!IsValid(AmmoData) && Reward.ItemAssetId.IsValid())
	{
		AmmoData = Cast<UPRAmmoDataAsset>(UPRAssetManager::Get().GetItemDataByPrimaryAssetId(Reward.ItemAssetId));
	}

	if (!IsValid(AmmoData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 지급 실패. AmmoData 없음. PlayerState = %s"), *GetNameSafe(PlayerState));
		return false;
	}

	UPRAbilitySystemComponent* ASC = PlayerState->GetPRAbilitySystemComponent();
	const UPRAttributeSet_Weapon* WeaponSet = IsValid(ASC) ? ASC->GetSet<UPRAttributeSet_Weapon>() : nullptr;
	if (!IsValid(WeaponSet))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Drop][Server] 탄약 지급 실패. WeaponSet 없음. PlayerState = %s"), *GetNameSafe(PlayerState));
		return false;
	}

	const EPRAmmoType AmmoType = AmmoData->GetAmmoType();
	const float CurrentReserveAmmo = WeaponSet->GetReserveAmmoByType(AmmoType);
	const float MaxReserveAmmo = WeaponSet->GetMaxReserveAmmoByType(AmmoType);
	const float GrantedAmmo = FMath::Min(static_cast<float>(Reward.Quantity), FMath::Max(MaxReserveAmmo - CurrentReserveAmmo, 0.0f));
	if (GrantedAmmo <= 0.0f)
	{
		return false;
	}

	const FGameplayAttribute ReserveAmmoAttribute = UPRAttributeSet_Weapon::GetReserveAmmoAttribute(AmmoType);
	ASC->SetNumericAttributeBase(ReserveAmmoAttribute, CurrentReserveAmmo + GrantedAmmo);

	FPRResolvedDropReward GrantedReward = Reward;
	GrantedReward.Quantity = FMath::RoundToInt(GrantedAmmo);
	NotifyPickupRewardGranted(PlayerState, GrantedReward);
	return true;
}

void UPRItemDropManagerSubsystem::NotifyPickupRewardGranted(APRPlayerState* PlayerState, const FPRResolvedDropReward& Reward) const
{
	APRPlayerController* PlayerController = IsValid(PlayerState)
		? Cast<APRPlayerController>(PlayerState->GetOwner())
		: nullptr;
	if (!IsValid(PlayerController))
	{
		return;
	}

	FPRPickupNotificationPayload Payload;
	Payload.RewardType = Reward.RewardType;
	Payload.ItemAssetId = Reward.ItemAssetId;
	Payload.Quantity = Reward.RewardType == EPRRewardType::Currency ? Reward.ScrapAmount : Reward.Quantity;
	if (Payload.Quantity <= 0)
	{
		return;
	}

	PlayerController->ClientNotifyPickupReward(Payload);
}
