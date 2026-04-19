// Copyright ProjectR. All Rights Reserved.

#include "PRPlayerState.h"
#include "Net/UnrealNetwork.h"

// =====  복제 등록 ===== 

void APRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APRPlayerState, DisplayName);
	DOREPLIFETIME(APRPlayerState, CharacterLevel);
	DOREPLIFETIME(APRPlayerState, Experience);
	DOREPLIFETIME(APRPlayerState, Stats);
}

// =====  초기화 ===== 

void APRPlayerState::InitializeFromSaveData(const FPRCharacterSaveData& SaveData)
{
	if (!HasAuthority())
	{
		return;
	}

	DisplayName    = SaveData.DisplayName;
	CharacterLevel = SaveData.Level;
	Experience     = SaveData.Experience;
	Stats          = SaveData.Stats;
}
