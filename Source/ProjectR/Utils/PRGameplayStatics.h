// Copyright (c) 2026 TeamApoc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PRGameplayStatics.generated.h"

class UPRInventoryComponent;
/**
 * 
 */
UCLASS()
class PROJECTR_API UPRGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 액터 및 모든 ChildActor를 재귀 탐색하여 UMeshComponent를 OutMeshes에 수집 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static void GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes);
	
	/** 액터의 InventoryComponent를 찾아 반환 */
	UFUNCTION(BlueprintCallable, Category = "PR|Utils")
	static UPRInventoryComponent* GetInventoryComponent(AActor* Actor);
};
