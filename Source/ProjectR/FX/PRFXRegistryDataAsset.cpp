// Copyright ProjectR. All Rights Reserved.

#include "PRFXRegistryDataAsset.h"

bool UPRFXRegistryDataAsset::FindEntry(FGameplayTag FXTag, FPRFXRegistryEntry& OutEntry) const
{
	if (!FXTag.IsValid())
	{
		return false;
	}

	if (const FPRFXRegistryEntry* FoundEntry = Entries.Find(FXTag))
	{
		// 호출 측의 Registry 내부 저장소 수정을 막기 위한 값 복사 반환
		OutEntry = *FoundEntry;
		return true;
	}

	return false;
}
