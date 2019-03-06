// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolHolder.h"
#include "PoolManager.generated.h"


UENUM(BlueprintType)
enum class EBranch : uint8 {
	Success		UMETA(DisplayName="Success"),
	Failed		UMETA(DisplayName="Failed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitializedPoolManager);

UCLASS()
class APoolManager : public AActor
{
	GENERATED_BODY()
	
public:

	// Instance for this singleton
	static APoolManager* Instance;

	UPROPERTY(BlueprintAssignable)
		FInitializedPoolManager OnInitialized;

	TMap<FString, APoolHolder*> ClassNamesToPools;

	// Sets default values for this actor's properties
	APoolManager();

	//UFUNCTION(BlueprintCallable, Meta = (WorldContext = "WorldContextObject", FriendlyName = "PoolTest", BlueprintInternalUseOnly = "true"))
		//static class AActor* PoolTestRealName(UObject* WorldContextObject, TSubclassOf<class AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get the singleton instance of the pool manager"))
		static APoolManager* GetPoolManager();

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (CustomStructureParam = "OptionalConstructionInformations", ToolTip = "Get a single object from the pool", DeterminesOutputType = "Class", Keywords = "Get Pool"))
		static UObject* GetFromPool(TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintCallable, Category = "Object Pool|Multiplayer", Meta = (ToolTip = "Get a specific object from the pool by its name", DeterminesOutputType = "Class", Keywords = "Pool Specific Name String"))
		static UObject* GetSpecificFromPool(TSubclassOf<UObject> Class, FString ObjectName);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get a specific object from the pool by its name, if the object isn't available the function will return a random object from the pool", DeterminesOutputType = "Class", Keywords = "Pool Specific Name String"))
		static UObject* TryToGetSpecificFromPool(TSubclassOf<UObject> Class, FString ObjectName);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get a variable number of objects from the pool", DeterminesOutputType = "Class", Keywords = "X Amount Number Quantity Pool"))
		static TArray<UObject*> GetXFromPool(TSubclassOf<UObject> Class, int32 Quantity = 10);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get all unused objects from the pool", DeterminesOutputType = "Class", Keywords = "All Pool"))
		static TArray<UObject*> GetAllFromPool(TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (AdvancedDisplay = "PoolOwner,PoolInstigator", ToolTip = "Use this function like SpawnActor, but instead of creating a new actor it will take an unused one from the pool", DeterminesOutputType = "Class", ExpandEnumAsExecs = "Branch", Keywords = "Spawn Pool Get"))
		static AActor* SpawnActorFromPool(TSubclassOf<AActor> Class, FTransform SpawnTransform, UPARAM(DisplayName = "Owner") AActor* PoolOwner, UPARAM(DisplayName = "Instigator") APawn* PoolInstigator, EBranch& Branch);

	UFUNCTION(BlueprintCallable, Category = "Object Pool|Multiplayer", Meta = (AdvancedDisplay = "PoolOwner,PoolInstigator", ToolTip = "Use this function like SpawnActor, but instead of creating a new actor it will take an unused one from the pool", DeterminesOutputType = "Class", ExpandEnumAsExecs = "Branch", Keywords = "Spawn Pool Get Multiplayer Network"))
		static AActor* SpawnSpecificActorFromPool(TSubclassOf<AActor> Class, FString ObjectName, FTransform SpawnTransform, UPARAM(DisplayName = "Owner") AActor* PoolOwner, UPARAM(DisplayName = "Instigator") APawn* PoolInstigator, EBranch& Branch);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (AdvancedDisplay = "PoolOwner,PoolInstigator", ToolTip = "If the specified actor is not available this function will return a random actor of the given class", DeterminesOutputType = "Class", ExpandEnumAsExecs = "Branch", Keywords = "Spawn Pool Get Multiplayer Network"))
		static AActor* TryToSpawnSpecificActorFromPool(TSubclassOf<AActor> Class, FString ObjectName, FTransform SpawnTransform, UPARAM(DisplayName = "Owner") AActor* PoolOwner, UPARAM(DisplayName = "Instigator") APawn* PoolInstigator, EBranch& Branch);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (DefaultToSelf = "Object", ToolTip = "Put an used object back to the pool", Keywords = "Return Back Pool"))
		static void ReturnToPool(UObject* Object, const EEndPlayReason::Type EndPlayReason = EEndPlayReason::Destroyed);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Clear a specific pool", Keywords = "Empty Clear Pool Destroy"))
		static void EmptyObjectPool(TSubclassOf<UObject> Class);

	//UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Create a new object pool. If you pass a class with an existing pool this will destroy all elements of the existing pool!", Keywords = "Init Create Start Pool"))
		//static void InitializeObjectPool(FPoolSpecification PoolSpecification);

	UFUNCTION(BlueprintPure, Category = "Object Pool|Multiplayer", Meta = (ToolTip = "Get the name of the object for the function 'GetSpecificFromPool'", DefaultToSelf = "Object", Keywords = "Object Pool"))
		static FString GetObjectName(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Get the amount of used objects of the pool"))
		static int32 GetNumberOfUsedObjects(TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Get the amount of unused objects of the pool"))
		static int32 GetNumberOfAvailableObjects(TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Returns true if the object is NOT a part of the available object pool", Keywords = "Active Object Pool"))
		static bool IsObjectActive(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Returns true if the object pool holds objects of the given class", Keywords = "Contains Object Pool"))
		static bool ContainsClass(TSubclassOf<UObject> Class);

protected:
	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "This will initialize all the pools defined by DesiredPools"))
		void InitializePools();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:

	UPROPERTY(EditAnywhere)
		TArray<FPoolSpecification> DesiredPools;

	bool bIsReady = false;

	void DestroyAllPools();

	static void InitializeObjectPool(FPoolSpecification PoolSpecification);

	/*
	* Return false if the PoolManager doesn't contain the specific poolholder
	*/
	static bool GetPoolHolder(TSubclassOf<UObject> Class, APoolHolder*& PoolHolder);

	static bool IsPoolManagerReady();
};
