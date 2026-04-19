// Copyright ProjectR. All Rights Reserved.

#include "PRInputConfigDataAsset.h"

const UInputAction* UPRInputConfigDataAsset::FindInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FPRInputActionBinding& Binding : AbilityInputBindings)
	{
		if (Binding.InputAction != nullptr && Binding.InputTag == InputTag)
		{
			return Binding.InputAction;
		}
	}
	return nullptr;
}
