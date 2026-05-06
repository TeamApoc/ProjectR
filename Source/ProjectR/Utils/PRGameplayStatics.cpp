// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGameplayStatics.h"

#include "ProjectR/Player/PRPlayerState.h"

void UPRGameplayStatics::GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 현재 액터의 모든 MeshComponent 수집
	Actor->GetComponents<UMeshComponent>(OutMeshes, /*bIncludeFromChildActors=*/false);

	// ChildActorComponent를 통한 자식 액터 재귀 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	Actor->GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* ChildActorComp : ChildActorComps)
	{
		if (!IsValid(ChildActorComp))
		{
			continue;
		}

		TArray<UMeshComponent*> ChildMeshes;
		GetAllMeshComponents(ChildActorComp->GetChildActor(), ChildMeshes);
		OutMeshes.Append(ChildMeshes);
	}

	// 부착된 액터 재귀 탐색
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, /*bResetArray=*/true, /*bRecursivelyIncludeAttachedActors=*/false);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		GetAllMeshComponents(AttachedActor, AttachedMeshes);
		OutMeshes.Append(AttachedMeshes);
	}
}

UPRInventoryComponent* UPRGameplayStatics::GetInventoryComponent(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	
	APawn* AsPawn = nullptr;
	
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		AsPawn = Pawn;
	}
	else if (AController* Controller = Cast<AController>(Actor))
	{
		AsPawn = Controller->GetPawn();
	}
	
	if (AsPawn)
	{
		if (APRPlayerState* PS = Cast<APRPlayerState>(AsPawn->GetPlayerState()))
		{
			return PS->GetInventoryComponent();
		}
	}
	
	if (APRPlayerState* PS = Cast<APRPlayerState>(Actor))
	{
		return PS->GetInventoryComponent();
	}
	
	return nullptr;
}
