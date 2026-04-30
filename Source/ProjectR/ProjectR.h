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
}