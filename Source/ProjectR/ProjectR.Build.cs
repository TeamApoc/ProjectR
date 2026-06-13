// Copyright Epic Games, Inc. All Rights Reserved.
// Author: 김동석 (로딩 화면 프리로드 및 오디오 모듈 종속성 설정)
// Author: 배유찬 (온라인 세션 및 어빌리티 시스템 종속성 설정)
// Author: 손승우 (AI 및 모션 워핑 모듈 종속성 설정)
using UnrealBuildTool;

public class ProjectR : ModuleRules
{
	public ProjectR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"DeveloperSettings",
			"NavigationSystem",
			"MotionWarping",
			"UMG",
			"NetCore",
			"StructUtils",
			"Niagara",
			"PhysicsCore",
			"Slate",
			"SlateCore",
			"OnlineSubsystem",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTracks",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime", "OnlineSubsystemUtils" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
