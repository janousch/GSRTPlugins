// Copyright 2019 (C) Ramón Janousch

#include "PoolManager.h"
#include "Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PoolHolder.h"

APoolManager* APoolManager::Instance;

// Sets default values
APoolManager::APoolManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Add a root component to stick the pool on the pool manager
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

// Called when the game starts or when spawned
void APoolManager::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;
	InitializePools();
}

APoolManager* APoolManager::GetPoolManager() {
	return Instance;
}

AActor* APoolManager::BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UClass* Class, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const bool Reconstruct, bool &SpawnSuccessful) {
	//if (!ObjectPool->IsValidLowLevel() || ObjectPool->IsPendingKill()) {
	if (!Class) {
		return nullptr; 
	}
	//const auto &Settings = GetMutableDefault<UPoolSettings>();

	//if (ObjectPool->Pool.Num() == 0) { 
		//ObjectPool->InitializeObjectPool(); 
	//}
	//AActor* DeferredSpawn = ObjectPool->GetInactiveObject();
	AActor* DeferredSpawn = Cast<AActor>(GetFromPool(Class, FSpawnParameter(), FSpecificSearch()));

	if (DeferredSpawn == nullptr) {// && Settings->InstantiateOnDemand) {
		//DeferredSpawn = Instance->GetWorld()->SpawnActorDeferred<AActor>(Class, SpawnTransform, Owner, ObjectPool->GetOwner()->GetInstigator(), CollisionHandlingOverride);
		DeferredSpawn = Instance->GetWorld()->SpawnActorDeferred<AActor>(Class, SpawnTransform);
		//if (DeferredSpawn) { 
			//DeferredSpawn->OwningPool = ObjectPool; DeferredSpawn->FinishSpawning(SpawnTransform); 
		//}
	}/*
	else if (DeferredSpawn == nullptr && Settings->NeverFailDeferredSpawn) {
		DeferredSpawn = ObjectPool->GetSpawnedObject();
		if (DeferredSpawn != nullptr) { 
			DeferredSpawn->ReturnToPool(); 
		}
	} 
	ObjectPool->FlushObjectPool();

	if (DeferredSpawn != nullptr) {
		const APawn* Instigator = Cast<APawn>(WorldContextObject);
		if (Instigator == nullptr) { if (const AActor* ContextActor = Cast<AActor>(WorldContextObject)) { 
			Instigator = ContextActor->Instigator; } 
		}
		if (Instigator != nullptr) { 
			DeferredSpawn->Instigator = const_cast<APawn*>(Instigator); 
		}
		if (Owner == nullptr) { 
			DeferredSpawn->SetOwner(ObjectPool->GetOwner()); 
		}

		DeferredSpawn->OwningPool = ObjectPool;
		DeferredSpawn->Initialize();
		DeferredSpawn->SpawnFromPool(Reconstruct, SpawnOptions, SpawnTransform);
		SpawnSuccessful = DeferredSpawn->Spawned;
	}
	else {
		SpawnSuccessful = false;
		DeferredSpawn = ObjectPool->GetSpawnedObject();
	} 
	*/
	
	return DeferredSpawn;
}


AActor* APoolManager::FinishDeferredSpawnFromPool(AActor* Actor, const FTransform& SpawnTransform) {
	if (Actor->IsValidLowLevel() && !Actor->IsActorInitialized() && !Actor->IsPendingKill()) {
		Actor->FinishSpawning(SpawnTransform);
	} return Actor;
}


UObject* APoolManager::GetFromPool(TSubclassOf<UObject> Class, FSpawnParameter SpawnParameter, FSpecificSearch SpecificSearch) {
	APoolHolder* PoolHolder;
	if (Instance->GetPoolHolder(Class, PoolHolder)) {
		if (PoolHolder->IsValidLowLevel()) {
			FString SpecificActor = SpecificSearch.SpecificActor;
			bool bReturnSpecificActor = SpecificActor.Len() > 0;

			UObject* UnusedObject;
			if (bReturnSpecificActor) {
				UnusedObject = PoolHolder->GetSpecific(SpecificActor);

				if (SpecificSearch.HandleNoSpecificFound == EHandleNoSpecificFound::NEXT_FREE && !UnusedObject->IsValidLowLevel()) {
					UnusedObject = PoolHolder->GetUnused();
				}
			}
			else {
				UnusedObject = PoolHolder->GetUnused();
			}

			if (UnusedObject == nullptr) {
				if (SpawnParameter.HandleEmptyPool == EHandleEmptyPool::CREATE) {
					if (Class->IsChildOf(AActor::StaticClass())) {
						UnusedObject = Instance->GetWorld()->SpawnActor(Class);
					}
					else {
						UnusedObject = NewObject<UObject>((UObject*)GetTransientPackage(), Class);
					}
				}
				else if (SpawnParameter.HandleEmptyPool == EHandleEmptyPool::CREATE_AND_ADD) {
					UnusedObject = PoolHolder->GetNew();
				}
			}

			if (SpawnParameter.bSetActive) {
				PoolHolder->SetObjectActive(UnusedObject);
			}

			return UnusedObject;
		}
	}
	
	return nullptr;
}

bool APoolManager::GetPoolHolder(TSubclassOf<UObject> Class, APoolHolder*& PoolHolder) {
	if (Class) {
		if (IsPoolManagerReady()) {
			FString Key = Class->GetName();
			if (ClassNamesToPools.Contains(Key)) {
				PoolHolder = *ClassNamesToPools.Find(Key);
				return true;
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("Pool Manager doesn't contain the class %s!"), *Key);
			}
		}

		UE_LOG(LogTemp, Error, TEXT("Pool Manager is not ready yet!"));
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Pass a valid class in which inherits from UObject!"));
	}

	return false;
}

TArray<UObject*> APoolManager::GetXFromPool(TSubclassOf<UObject> Class, int32 Quantity) {
	TArray<UObject*> Objects;
	for (int i = 0; i < Quantity; i++) {
		UObject* Object = GetFromPool(Class, FSpawnParameter(), FSpecificSearch());
		if (Object->IsValidLowLevel()) break;
		
		Objects.Add(Object);
	}

	return Objects;
}

TArray<UObject*> APoolManager::GetAllFromPool(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (Instance->GetPoolHolder(Class, PoolHolder)) {
		if (PoolHolder->IsValidLowLevel()) {
			return PoolHolder->GetAllUnused();
		}
	}

	return TArray<UObject*>();
}

void APoolManager::SetPoolObjectActive(UObject* Object, bool bSetActive) {
	if (!Object->IsValidLowLevel()) return;
	if (!IsPoolManagerReady()) return;

	APoolHolder* PoolHolder;
	if (Instance->GetPoolHolder(Object->GetClass(), PoolHolder)) {
		if (PoolHolder->IsValidLowLevel()) {
			PoolHolder->SetObjectActive(Object, bSetActive);
		}
	}
}

AActor* APoolManager::SpawnActorFromPool(TSubclassOf<AActor> Class, FTransform SpawnTransform, AActor* PoolOwner, APawn* PoolInstigator, EBranch& Branch, FSpawnParameter SpawnParameter, FSpecificSearch SpecificSearch) {
	if (Class) {
		AActor* UnusedActor = Cast<AActor>(GetFromPool(Class, SpawnParameter, SpecificSearch));

		if (!UnusedActor->IsValidLowLevel()) {
			Branch = EBranch::Failed;
			return nullptr;
		}

		UnusedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		UnusedActor->SetOwner(PoolOwner);
		UnusedActor->Instigator = PoolInstigator;

		Branch = EBranch::Success;
		return UnusedActor;
	}

	UE_LOG(LogTemp, Error, TEXT("Pass a valid class in SpawnActorFromPool which inherits from Actor!"));
	Branch = EBranch::Failed;
	return nullptr;
}

void APoolManager::InitializePools() {
	DestroyAllPools();

	FString Context;
	TArray<FPoolEntry*> PoolEntries;
	DataTable->GetAllRows<FPoolEntry>(Context, PoolEntries);

	for (auto& PoolEntry : PoolEntries) {
		FPoolEntry Entry = *PoolEntry;
		InitializeObjectPool(Entry);
	}

	bIsReady = true;
	OnInitialized.Broadcast();
}

void APoolManager::ReturnToPool(UObject* Object) {
	APoolHolder* PoolHolder;
	if (!Instance->GetPoolHolder(Object->GetClass(), PoolHolder)) return;
	if (!IsValid(PoolHolder)) return;
	PoolHolder->ReturnObject(Object);
}

void APoolManager::EmptyObjectPool(TSubclassOf<UObject> Class) {
	if (Class) {
		if (!IsValid(Instance)) return;
		if (Instance->ClassNamesToPools.Num() == 0) return;
		Instance->bIsReady = false;

		APoolHolder* PoolHolder = *Instance->ClassNamesToPools.Find(*Class->GetName());
		if (!IsValid(PoolHolder)) return;

		PoolHolder->Destroy();
		Instance->ClassNamesToPools.Remove(*Class->GetName());
		Instance->bIsReady = true;
	}
}

void APoolManager::InitializeObjectPool(FPoolEntry PoolEntry) {
	APoolHolder* PoolHolder = GetWorld()->SpawnActor<APoolHolder>(APoolHolder::StaticClass(), GetTransform());
	PoolHolder->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

	PoolHolder->InitializePool(PoolEntry);
	ClassNamesToPools.Add(PoolEntry.Class->GetName(), PoolHolder);
}

FString APoolManager::GetObjectName(UObject* Object) {
	if (!Object->IsValidLowLevel()) return "None";

	FString FullName = Object->GetFullName();
	FString Path;
	FString Name;
	FullName.Split(":PersistentLevel.", &Path, &Name);

	return Name;
}

int32 APoolManager::GetNumberOfUsedObjects(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (!Instance->GetPoolHolder(Class, PoolHolder)) return -1;
	if (!IsValid(PoolHolder)) return -1;

	return PoolHolder->GetNumberOfUsedObjects();
}

int32 APoolManager::GetNumberOfAvailableObjects(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (!Instance->GetPoolHolder(Class, PoolHolder)) return -1;
	if (!IsValid(PoolHolder)) return -1;

	return PoolHolder->GetNumberOfAvailableObjects();
}

bool APoolManager::IsObjectActive(UObject* Object) {
	AActor* Actor = Cast<AActor>(Object);
	if (Actor->IsValidLowLevel()) {
		AActor* AttachedTo = Actor->GetAttachParentActor();
		if (AttachedTo->IsValidLowLevel()) {
			return AttachedTo->GetClass()->IsChildOf(APoolHolder::StaticClass());
		}
	}
	else {
		APoolHolder* PoolHolder;
		if (Instance->GetPoolHolder(Object->GetClass(), PoolHolder)) {
			if (PoolHolder->IsValidLowLevel()) {
				return !PoolHolder->IsObjectAvailable(Object);
			}
		}
	}

	return false;
}

bool APoolManager::ContainsClass(TSubclassOf<UObject> Class) {
	if (!Class) return false;
	FString Key = Class->GetName();
	return Instance->ClassNamesToPools.Contains(Key);
}

void APoolManager::DestroyAllPools() {
	if (!IsPoolManagerReady()) return;
	
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (auto& Actor : AttachedActors) {
		Actor->Destroy();
	}

	TArray<APoolHolder*> Pools;
	ClassNamesToPools.GenerateValueArray(Pools);
	for (auto& PoolHolder : Pools) {
		if (!PoolHolder->IsPendingKill()) {
			PoolHolder->Destroy();
		}
	}
}

bool APoolManager::IsPoolManagerReady() {
	if (Instance == nullptr) return false;
	if (!Instance->IsValidLowLevel()) return false;
	if (Instance->ClassNamesToPools.Num() == 0) return false;
	if (!Instance->bIsReady) return false;

	return true;
}

void APoolManager::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	DestroyAllPools();
	Super::EndPlay(EndPlayReason);
}