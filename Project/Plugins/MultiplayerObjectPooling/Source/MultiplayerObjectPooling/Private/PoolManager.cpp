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
	Instance = this;
	Super::BeginPlay();
	InitializePools();
}

/*
class AActor* APoolManager::PoolTestRealName(UObject* WorldContextObject, TSubclassOf<class AActor> ActorClass) {
	return Cast<AActor>(NewObject<UObject>((UObject*)GetTransientPackage(), ActorClass));
}

template< class T >
T* CreateDataItem(APlayerController* OwningPlayer, UClass* UserWidgetClass)
{
	if (!UserWidgetClass->IsChildOf(UGISItemData::StaticClass()))
	{
		return nullptr;
	}

	// Assign the outer to the game instance if it exists, otherwise use the player controller's world
	UWorld* World = OwningPlayer->GetWorld();
	StaticCast<UObject*>(World);
	UGISItemData* NewWidget = NewObject<UObject>((UObject*)GetTransientPackage(), Test);
	return Cast<T>(NewWidget);
}
*/

APoolManager* APoolManager::GetPoolManager() {
	return Instance;
}

UObject* APoolManager::GetFromPool(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (GetPoolHolder(Class, PoolHolder)) {
		if (PoolHolder->IsValidLowLevelFast()) {
			UObject* UnusedObject = PoolHolder->GetUnused();
			return UnusedObject;
		}
	}

	return nullptr;
}

UObject* APoolManager::GetSpecificFromPool(TSubclassOf<UObject> Class, FString ObjectName) {
	APoolHolder* PoolHolder;
	if (GetPoolHolder(Class, PoolHolder)) {
		if (PoolHolder->IsValidLowLevelFast()) {
			return PoolHolder->GetSpecific(ObjectName);
		}
	}

	return nullptr;
}

UObject* APoolManager::TryToGetSpecificFromPool(TSubclassOf<UObject> Class, FString ObjectName) {
	UObject* PoolObject = GetSpecificFromPool(Class, ObjectName);
	if (PoolObject->IsValidLowLevelFast()) return PoolObject;

	return GetFromPool(Class);
}

bool APoolManager::GetPoolHolder(TSubclassOf<UObject> Class, APoolHolder*& PoolHolder) {
	if (Class) {
		if (IsPoolManagerReady()) {
			FString Key = Class->GetName();
			if (Instance->ClassNamesToPools.Contains(Key)) {
				PoolHolder = *Instance->ClassNamesToPools.Find(Key);
				return true;
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
		UObject* Object = GetFromPool(Class);
		if (Object->IsValidLowLevelFast()) break;
		
		Objects.Add(Object);
	}

	return Objects;
}

TArray<UObject*> APoolManager::GetAllFromPool(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (GetPoolHolder(Class, PoolHolder)) {
		if (PoolHolder->IsValidLowLevelFast()) {
			return PoolHolder->GetAllUnused();
		}
	}

	return TArray<UObject*>();
}

AActor* APoolManager::SpawnSpecificActorFromPool(TSubclassOf<AActor> Class, FString ObjectName, FTransform SpawnTransform, AActor* PoolOwner, APawn* PoolInstigator, EBranch& Branch) {
	if (Class) {
		AActor* UnusedActor = (AActor*)GetSpecificFromPool(Class, ObjectName);
		if (!IsValid(UnusedActor)) {
			Branch = EBranch::Failed;
			return NULL;
		}

		UnusedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		UnusedActor->SetOwner(PoolOwner);
		UnusedActor->Instigator = PoolInstigator;

		Branch = EBranch::Success;
		return UnusedActor;
	}

	UE_LOG(LogTemp, Error, TEXT("Pass a valid class in SpawnActorFromPool which inherits from Actor!"));
	Branch = EBranch::Failed;
	return NULL;
}

AActor* APoolManager::TryToSpawnSpecificActorFromPool(TSubclassOf<AActor> Class, FString ObjectName, FTransform SpawnTransform, AActor* PoolOwner, APawn* PoolInstigator, EBranch& Branch) {
	AActor* PoolActor = SpawnSpecificActorFromPool(Class, ObjectName, SpawnTransform, PoolOwner, PoolInstigator, Branch);
	if (PoolActor->IsValidLowLevelFast()) {
		Branch = EBranch::Success;
		return PoolActor;
	}

	return SpawnActorFromPool(Class, SpawnTransform, PoolOwner, PoolInstigator, Branch);
}

AActor* APoolManager::SpawnActorFromPool(TSubclassOf<AActor> Class, FTransform SpawnTransform, AActor* PoolOwner, APawn* PoolInstigator, EBranch& Branch) {
	if (Class) {
		AActor* UnusedActor = (AActor*) GetFromPool(Class);
		if (!IsValid(UnusedActor)) {
			Branch = EBranch::Failed;
			return NULL;
		}

		UnusedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		UnusedActor->SetOwner(PoolOwner);
		UnusedActor->Instigator = PoolInstigator;

		Branch = EBranch::Success;
		return UnusedActor;
	}

	UE_LOG(LogTemp, Error, TEXT("Pass a valid class in SpawnActorFromPool which inherits from Actor!"));
	Branch = EBranch::Failed;
	return NULL;
}

void APoolManager::InitializePools() {
	DestroyAllPools();

	for (auto& PoolSpecification : DesiredPools) {
		InitializeObjectPool(PoolSpecification);
	}

	bIsReady = true;
	OnInitialized.Broadcast();
}

void APoolManager::ReturnToPool(UObject* Object, const EEndPlayReason::Type EndPlayReason) {
	APoolHolder* PoolHolder;
	if (!GetPoolHolder(Object->GetClass(), PoolHolder)) return;
	if (!IsValid(PoolHolder)) return;
	PoolHolder->ReturnObject(Object, EndPlayReason);
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

void APoolManager::InitializeObjectPool(FPoolSpecification PoolSpecification) {
	if (!IsValid(Instance)) {
		UE_LOG(LogTemp, Error, TEXT("Please spawn a PoolManager"));
		return;
	}

	APoolHolder* PoolHolder = Instance->GetWorld()->SpawnActor<APoolHolder>(APoolHolder::StaticClass(), Instance->GetTransform());
	PoolHolder->AttachToActor(Instance, FAttachmentTransformRules::KeepWorldTransform);

	PoolHolder->InitializePool(PoolSpecification);
	Instance->ClassNamesToPools.Add(PoolSpecification.Class->GetName(), PoolHolder);
}

FString APoolManager::GetObjectName(UObject* Object) {
	if (!Object->IsValidLowLevelFast()) return "None";

	FString FullName = Object->GetFullName();
	FString Path;
	FString Name;
	FullName.Split(":PersistentLevel.", &Path, &Name);

	return Name;
}

int32 APoolManager::GetNumberOfUsedObjects(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (!GetPoolHolder(Class, PoolHolder)) return -1;
	if (!IsValid(PoolHolder)) return -1;

	return PoolHolder->GetNumberOfUsedObjects();
}

int32 APoolManager::GetNumberOfAvailableObjects(TSubclassOf<UObject> Class) {
	APoolHolder* PoolHolder;
	if (!GetPoolHolder(Class, PoolHolder)) return -1;
	if (!IsValid(PoolHolder)) return -1;

	return PoolHolder->GetNumberOfAvailableObjects();
}

bool APoolManager::IsObjectActive(UObject* Object) {
	AActor* Actor = Cast<AActor>(Object);
	if (Actor->IsValidLowLevelFast()) {
		AActor* AttachedTo = Actor->GetAttachParentActor();
		if (AttachedTo->IsValidLowLevelFast()) {
			return AttachedTo->GetClass()->IsChildOf(APoolHolder::StaticClass());
		}
	}
	else {
		APoolHolder* PoolHolder;
		if (GetPoolHolder(Object->GetClass(), PoolHolder)) {
			if (PoolHolder->IsValidLowLevelFast()) {
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
	TArray<APoolHolder*> Pools;
	ClassNamesToPools.GenerateValueArray(Pools);

	for (auto& PoolHolder : Pools) {
		PoolHolder->Destroy();
	}

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (auto& Actor : AttachedActors) {
		Actor->Destroy();
	}
}

bool APoolManager::IsPoolManagerReady() {
	if (!IsValid(Instance)) return false;
	if (Instance->ClassNamesToPools.Num() == 0) return false;
	if (!Instance->bIsReady) return false;

	return true;
}