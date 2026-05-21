// Copyright ProjectR. All Rights Reserved.

#include "PRCurrencyComponent.h"

#include "Net/UnrealNetwork.h"
#include "ProjectR/Game/PRGameTypes.h"

UPRCurrencyComponent::UPRCurrencyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPRCurrencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UPRCurrencyComponent, Scrap, COND_OwnerOnly);
}

bool UPRCurrencyComponent::AddScrap(int32 Amount)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Currency][Server] AddScrap 실패. Owner = %s | Amount = %d | CurrentScrap = %d"),
			*GetNameSafe(GetOwner()),
			Amount,
			Scrap);
		return false;
	}

	const int32 PreviousScrap = Scrap;
	Scrap += Amount;
	OnScrapChanged.Broadcast(Scrap);
	GetOwner()->ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Currency][Server] AddScrap 완료. Owner = %s | Amount = %d | BeforeScrap = %d | AfterScrap = %d"),
		*GetNameSafe(GetOwner()),
		Amount,
		PreviousScrap,
		Scrap);

	return true;
}

bool UPRCurrencyComponent::TrySpendScrap(int32 Amount)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || Amount <= 0 || Scrap < Amount)
	{
		return false;
	}

	Scrap -= Amount;
	OnScrapChanged.Broadcast(Scrap);
	GetOwner()->ForceNetUpdate();
	return true;
}

FPRCurrencySaveData UPRCurrencyComponent::MakeSaveData() const
{
	// 재화 스냅샷
	FPRCurrencySaveData SaveData;
	SaveData.Scrap = Scrap;
	return SaveData;
}

void UPRCurrencyComponent::ApplySaveData(const FPRCurrencySaveData& InSaveData)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 고철 보유량 복원
	Scrap = FMath::Max(InSaveData.Scrap, 0);
	OnScrapChanged.Broadcast(Scrap);
	GetOwner()->ForceNetUpdate();
}

void UPRCurrencyComponent::OnRep_Scrap()
{
	OnScrapChanged.Broadcast(Scrap);
}
