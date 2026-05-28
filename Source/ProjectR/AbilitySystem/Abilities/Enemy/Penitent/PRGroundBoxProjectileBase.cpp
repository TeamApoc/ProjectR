// Copyright (c) 2026 TeamApoc. All Rights Reserved.


#include "PRGroundBoxProjectileBase.h"


// Sets default values
APRGroundBoxProjectileBase::APRGroundBoxProjectileBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APRGroundBoxProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APRGroundBoxProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

