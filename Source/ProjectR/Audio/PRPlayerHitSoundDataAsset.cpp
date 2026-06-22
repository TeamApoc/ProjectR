// Copyright ProjectR. All Rights Reserved.
// Author: 김동석 (플레이어 체력 피해 사운드 데이터 에셋 구현)
#include "PRPlayerHitSoundDataAsset.h"

#include "Sound/SoundBase.h"

USoundBase* UPRPlayerHitSoundDataAsset::ResolveHealthDamageSound(FName SourceCharacterID) const
{
	if (!SourceCharacterID.IsNone())
	{
		if (const TObjectPtr<USoundBase>* FoundSound = HealthDamageSoundsByCharacterID.Find(SourceCharacterID))
		{
			if (IsValid(FoundSound->Get()))
			{
				return FoundSound->Get();
			}
		}
	}

	return DefaultHealthDamageSound.Get();
}
