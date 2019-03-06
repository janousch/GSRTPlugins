// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "PoolableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UPoolableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Use the interface functions instead of BeginPlay and EndPlay.
 */
class IPoolableInterface
{
	GENERATED_BODY()

public:

	/*
	* This function gets called when the object is pulled out of the pool
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Object Pool", Meta = (Tooltip = "Use this function instead of BeginPlay"))
		void PoolableBeginPlay();

	/*
	* Gets called when the object returns to the pool
	* @param EndPlayReason
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Object Pool", Meta = (Tooltip = "Use this function instead of EndPlay"))
		void PoolableEndPlay(const EEndPlayReason::Type EndPlayReason);

	/*
	* Gets called when the object is pulled out from the pool and construction informations are provided
	* @param EndPlayReason
	*/
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Object Pool", Meta = (CustomStructureParam = "ConstructionInformations", Tooltip = "Use this function instead of ConstructionBlueprint"))
		void PoolableConstruction();
};
