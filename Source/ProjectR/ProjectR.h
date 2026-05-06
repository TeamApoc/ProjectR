// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

namespace PRRowNames
{
	namespace Player
	{
		const FName Default = "Default";	
	}
	
	namespace Enemies
	{
		const FName Faerin = "Faerin";
		// TODO: Add Enemy Names
	}
}

namespace PRCollisionChannels
{
	constexpr ECollisionChannel ECC_Combat = ECC_GameTraceChannel1;
	constexpr ECollisionChannel ECC_Projectile = ECC_GameTraceChannel2;
	constexpr ECollisionChannel ECC_Ground = ECC_GameTraceChannel3;
	constexpr ECollisionChannel ECC_Interactable = ECC_GameTraceChannel4;
}

namespace PRStencilValues
{
	constexpr int32 Default = 0;
	constexpr int32 Highlight= 1;
	constexpr int32 Interaction = 2;
	constexpr int32 Friendly = 3;
	constexpr int32 Hostile = 4;
}