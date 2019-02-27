// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoolHolder.generated.h"

USTRUCT(BlueprintType, Category = "Object Pool")
struct FPoolSpecification {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "Any class which inherites from UObject"))
		TSubclassOf<UObject> Class;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "The number of objects you want to have inside the pool"))
		int32 NumberOfObjects;
};

// Used to remember the default object settings
USTRUCT()
struct FDefaultObjectSettings {
	GENERATED_BODY()

public:
	// Object Settings

	bool bImplementsPoolableInterface;

	// Actor Settings

	bool bIsActor;
	bool bStartWithTickEnabled;
	float TickInterval;
	bool bHiddenInGame;
	float LifeSpan;
	bool bCanBeDamaged;

};

// Used to remember the default objects components settings
USTRUCT()
struct FDefaultComponentSettings {
	GENERATED_BODY()

public:
	// ActorComponent Settings

	bool bImplementsPoolableInterface;
	bool bStartWithTickEnabled;
	float TickInterval;
	TArray<FName> Tags;
	bool bAutoActivate;

	// SceneComponent Settings
	bool bIsSceneComponent;
	FTransform RelativeTransform;
	bool bIsVisible;
	bool bIsHidden;

	// StaticMeshComponent Settings
	bool bIsStaticMeshComponent;
	bool bIsSimulatingPhysics;
};

/**
 * Stores all the objects inside the specified pool
 */
UCLASS()
class APoolHolder : public AActor
{
	GENERATED_BODY()
	
public:

	APoolHolder();

	// Add a new object to the pool and deactivate it
	void Add(UObject* Object);

	// Get an unused object from the pool
	UObject* GetUnused();

	// Get all unused objects from the pool
	TArray<UObject*> GetAllUnused();

	// Get a specific object by its name
	UObject* GetSpecific(FString ObjectName);

	int32 GetNumberOfUsedObjects();

	int32 GetNumberOfAvailableObjects();

	// Return an object to the pool
	UFUNCTION()
	void ReturnObject(UObject* Object, const EEndPlayReason::Type EndPlayReason);

	// Initialize the pool with a given class and the amount of objects that the pool will contain
	void InitializePool(FPoolSpecification PoolSpecification);

	bool IsObjectAvailable(UObject* Object);

	virtual void Destroyed() override;

private:

	// Contains all the objects of this pool
	UPROPERTY(EditAnywhere)
		TMap<FString, UObject*> ObjectPool;

	// Contains all available object names
	UPROPERTY(EditAnywhere)
	TArray<FString> AvailableObjects;

	// Saves the default object settings to restore them, when the object is pulled from the pool
	FDefaultObjectSettings DefaultObjectSettings;

	// Saves the default object components settings to restore them, when the object is pulled from the pool
	TArray<FDefaultComponentSettings> DefaultComponentsSettings;

	// This is only used when the object has a life span and triggers the return to pool function when the object dies
	TMap<UObject*, FTimerHandle> ObjectsToTimers;
	
	/* 
	* Activate or deactivate the object. On activation it will restore the default values
	* @param Object
	* @param bIsActive
	* @param EndPlayReason - will be passed to the EndPlay Interface function if implemented
	*/
	void SetObjectActive(UObject* Object, bool bIsActive = true, const EEndPlayReason::Type EndPlayReason = EEndPlayReason::Destroyed);

	/*
	* Get the specific object, set it active but don't remove it from the AvailableObjects array.
	* @param ObjectName
	* @return The specific Object
	*/
	UObject* GetSpecificAndSetActive(FString ObjectName);

	void RestoreActorSettings(AActor* Actor);

};
