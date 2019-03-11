// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoolHolder.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "PoolManager.generated.h"


UENUM(BlueprintType)
enum class EBranch : uint8 {
	Success		UMETA(ToolTip = "Successfully returned an actor."),
	Failed		UMETA(ToolTip = "Failed to return an actor!")
};

UENUM(BlueprintType)
enum class EHandleEmptyPool : uint8 {
	IGNORE				UMETA(DisplayName = "Ignore", ToolTip = "Ignore that no actor is available in the pool. No actor will be returned!"),
	CREATE				UMETA(DisplayName = "Create", ToolTip = "Create a new actor and return it."),
	CREATE_AND_ADD		UMETA(DisplayName = "CreateAndAdd", ToolTip = "Create and add a new actor to the pool, this actor will be returned.")
};

UENUM(BlueprintType)
enum class EHandleNoSpecificFound : uint8 {
	IGNORE				UMETA(DisplayName = "Ignore", ToolTip = "Ignore that no actor is abailable in the pool. No actor will be returned!"),
	NEXT_FREE			UMETA(DisplayName = "NextFree", ToolTip = "If the specific actor can't be found inside the pool the next free actor will be returned!")
};

USTRUCT(BlueprintType, Category = "Object Pool")
struct FSpawnParameter {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "Set the actor active (All the default values of the actor will be reseted (Tick enabled, Visibility and Collision settings)"))
		bool bSetActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "What has to happen when the pool is empty?"))
		EHandleEmptyPool HandleEmptyPool = EHandleEmptyPool::CREATE_AND_ADD;
};

USTRUCT(BlueprintType, Category = "Object Pool")
struct FSpecificSearch {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "Set an specific actor name to spawn from the pool"))
		FString SpecificActor = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ToolTip = "What has to happen when the specified actor can't be found?"))
		EHandleNoSpecificFound HandleNoSpecificFound = EHandleNoSpecificFound::NEXT_FREE;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitializedPoolManager);

UCLASS(MinimalAPI)
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

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get the singleton instance of the pool manager"))
		static APoolManager* GetPoolManager();

	//UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get a single object from the pool", DeterminesOutputType = "Class", Keywords = "Get Pool"))
		static UObject* GetFromPool(TSubclassOf<UObject> Class, FSpawnParameter SpawnParameter, FSpecificSearch SpecificSearch);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (AdvancedDisplay = "PoolOwner,PoolInstigator,SpawnParameter,SpecificSearch", ToolTip = "Use this function like SpawnActor, but instead of creating a new actor it will take an unused one from the pool", DeterminesOutputType = "Class", ExpandEnumAsExecs = "Branch", Keywords = "Spawn Pool Get"))
		static AActor* SpawnActorFromPool(TSubclassOf<AActor> Class, FTransform SpawnTransform, UPARAM(DisplayName = "Owner") AActor* PoolOwner, UPARAM(DisplayName = "Instigator") APawn* PoolInstigator, EBranch& Branch, FSpawnParameter SpawnParameter, FSpecificSearch SpecificSearch);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Set the actor active (reconstruct default values, visibility, tick)", DisplayName = "SetActive"))
		static void SetPoolObjectActive(UObject* Object, bool bSetActive = true);

	UFUNCTION(BlueprintPure, Category = "Object Pool|Multiplayer", Meta = (ToolTip = "Get the name of the object for the function 'GetSpecificFromPool'", DefaultToSelf = "Object", Keywords = "Object Pool", DisplayName = "GetName"))
		static FString GetObjectName(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (DefaultToSelf = "Object", ToolTip = "Put an used object back to the pool", Keywords = "Return Back Pool Destroy", DisplayName = "Destroy"))
		static void ReturnToPool(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Returns true if the object is NOT a part of the available object pool", Keywords = "Active Object Pool", DisplayName = "IsActive?"))
		static bool IsObjectActive(UObject* Object);

	//UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Clear a specific pool", Keywords = "Empty Clear Pool Destroy"))
		static void EmptyObjectPool(TSubclassOf<UObject> Class);

	//UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Get the amount of used objects of the pool"))
		static int32 GetNumberOfUsedObjects(TSubclassOf<UObject> Class);

	//UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Get the amount of unused objects of the pool"))
		static int32 GetNumberOfAvailableObjects(TSubclassOf<UObject> Class);

	//UFUNCTION(BlueprintPure, Category = "Object Pool", Meta = (ToolTip = "Returns true if the object pool holds objects of the given class", Keywords = "Contains Object Pool"))
		static bool ContainsClass(TSubclassOf<UObject> Class);

	//UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get a variable number of objects from the pool", DeterminesOutputType = "Class", Keywords = "X Amount Number Quantity Pool"))
		static TArray<UObject*> GetXFromPool(TSubclassOf<UObject> Class, int32 Quantity = 10);

	//UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "Get all unused objects from the pool", DeterminesOutputType = "Class", Keywords = "All Pool"))
		static TArray<UObject*> GetAllFromPool(TSubclassOf<UObject> Class);

	/// Spawns an Actor from Pool, manually running its Construction Scripts if a full reset is needed.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, Meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
		static AActor* BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UClass* Class, const FTransform &SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const bool Reconstruct, bool &SpawnSuccessful);
	//static AActor* BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UClass* Class, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful);

	/// Finishes Deferred Spawning an Actor from Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, Meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
		static AActor* FinishDeferredSpawnFromPool(AActor* Actor, const FTransform &SpawnTransform);


protected:
	UFUNCTION(BlueprintCallable, Category = "Object Pool", Meta = (ToolTip = "This will initialize all the pools defined by DesiredPools"))
		void InitializePools();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:

	UPROPERTY(EditInstanceOnly, Meta = (ToolTip = "The desired pools. The PoolManager will create these pools on BeginPlay! (Create a data table with the struct PoolEntry)"))
		UDataTable* DataTable;

	bool bIsReady = false;

	void DestroyAllPools();

	static void InitializeObjectPool(FPoolEntry PoolEntry);

	/*
	* Return false if the PoolManager doesn't contain the specific poolholder
	*/
	static bool GetPoolHolder(TSubclassOf<UObject> Class, APoolHolder*& PoolHolder);

	static bool IsPoolManagerReady();
};
