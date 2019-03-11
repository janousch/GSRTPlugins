// Copyright 2019 (C) Ramón Janousch

#include "PoolHolder.h"
#include "Engine.h"
#include "PoolableInterface.h"


APoolHolder::APoolHolder() {
	PrimaryActorTick.bCanEverTick = false;
	// Add a root component to stick the pool on the pool manager
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void APoolHolder::Add(UObject* Object) {
	FString Name = Object->GetName();
	ObjectPool.Add(Name, Object);
	AvailableObjects.Add(Name);

	SetObjectActive(Object, false);

	// Add a timer and ignore the life span for actors with a life span
	if (DefaultObjectSettings.LifeSpan > 0) {
		Cast<AActor>(Object)->SetLifeSpan(0);
		ObjectsToTimers.Add(Object, FTimerHandle());
	}
}

UObject* APoolHolder::GetUnused() {
	if (AvailableObjects.Num() > 0) {
		UObject* UnusedObject = GetSpecific(AvailableObjects.Pop(true));	

		return UnusedObject;
	}
	else {
		return nullptr;
	}
}

UObject* APoolHolder::GetNew() {
	if (DefaultObjectSettings.bIsActor) {
		AActor* NewActor = GetWorld()->SpawnActor(DefaultObjectSettings.Class);
		Add(NewActor);
	}
	else {
		Add(NewObject<UObject>((UObject*)GetTransientPackage(), DefaultObjectSettings.Class));
	}

	return GetUnused();
}

TArray<UObject*> APoolHolder::GetAllUnused() {
	TArray<UObject*> Objects;
	for (int i = 0; i < AvailableObjects.Num(); i++) {
		Objects.Add(GetSpecific(AvailableObjects[i]));
	}
	AvailableObjects.Empty();

	return Objects;
}

UObject* APoolHolder::GetSpecific(FString ObjectName) {
	// TODO: Bad performance, find a better way
	AvailableObjects.Remove(ObjectName);

	UObject** ObjectFromPool = ObjectPool.Find(ObjectName);

	if (ObjectFromPool == nullptr) return nullptr;

	UObject* UnusedObject = *ObjectFromPool;

	return UnusedObject;
}

void APoolHolder::ReturnObject(UObject* Object) {
	FString ObjectName = Object->GetName();
	AvailableObjects.Add(ObjectName);

	SetObjectActive(Object, false);
}

void APoolHolder::SetObjectActive(UObject* Object, bool bIsActive) {
	if (!IsValid(Object)) return;

	if (DefaultObjectSettings.bIsActor) {
		AActor* Actor = Cast<AActor>(Object);

		// Attach and detach the actor on the pool for better readability inside the editor
		if (bIsActive) {
			RestoreActorSettings(Actor);
			if (Actor->GetAttachParentActor() == this) {
				Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
		else {
			Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		}

		Actor->SetActorHiddenInGame(!bIsActive || DefaultObjectSettings.bHiddenInGame);
		Actor->SetActorEnableCollision(bIsActive);
		Actor->SetActorTickEnabled(bIsActive && DefaultObjectSettings.bStartWithTickEnabled);
	}

	if (DefaultObjectSettings.bImplementsPoolableInterface && bIsPoolHolderInitialized) {
		if (bIsActive) {
			IPoolableInterface::Execute_PoolableBeginPlay(Object);
		}
		else {
			IPoolableInterface::Execute_PoolableEndPlay(Object);
		}
	}
}

void APoolHolder::RestoreActorSettings(AActor* Actor) {
	// Restore default settings
	Actor->SetActorTickInterval(DefaultObjectSettings.TickInterval);
	Actor->bCanBeDamaged = DefaultObjectSettings.bCanBeDamaged;
	if (DefaultObjectSettings.LifeSpan > 0) {
		FTimerHandle* Timer = ObjectsToTimers.Find(Actor);
		FTimerDelegate TimerDel;
		TimerDel.BindUFunction(this, FName("ReturnObject"), Actor);
		GetWorldTimerManager().SetTimer(*Timer, TimerDel, DefaultObjectSettings.LifeSpan, false);
	}

	// Restore default components settings
	if (DefaultComponentsSettings.Num() > 0) {
		TArray<UActorComponent*> ActorComponents;
		Actor->GetComponents<UActorComponent>(ActorComponents);

		for (int i = 0; i < ActorComponents.Num(); i++) {
			if (i >= DefaultComponentsSettings.Num()) return;

			// Restore actor component settings
			UActorComponent* ActorComponent = ActorComponents[i];
			FDefaultComponentSettings ComponentSettings = DefaultComponentsSettings[i];

			ActorComponent->SetComponentTickEnabled(ComponentSettings.bStartWithTickEnabled);
			ActorComponent->SetComponentTickInterval(ComponentSettings.TickInterval);
			ActorComponent->ComponentTags = ComponentSettings.Tags;
			ActorComponent->SetActive(ComponentSettings.bAutoActivate);

			// Restore scene component settings
			if (ComponentSettings.bIsSceneComponent) {
				USceneComponent* SceneComponent = Cast<USceneComponent>(ActorComponent);
				if (IsValid(SceneComponent)) {
					if (i > 0) { // Skip the root transform
						SceneComponent->SetRelativeTransform(ComponentSettings.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
					}
					SceneComponent->SetVisibility(ComponentSettings.bIsVisible);
					SceneComponent->SetHiddenInGame(ComponentSettings.bIsHidden);

					// Restore static mesh component settings
					if (ComponentSettings.bIsStaticMeshComponent) {
						UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(SceneComponent);
						if (IsValid(StaticMeshComponent)) {
							StaticMeshComponent->SetSimulatePhysics(ComponentSettings.bIsSimulatingPhysics);
						}
					}
				}
			}
		}
	}
}

void APoolHolder::InitializePool(FPoolEntry PoolEntry) {
	bIsPoolHolderInitialized = false;
	TSubclassOf<UObject> Class = PoolEntry.Class;
	int32 NumberOfObjects = PoolEntry.AmountOfObjects;

	if (Class) {
		// Save the default object settings
		DefaultObjectSettings.bImplementsPoolableInterface = Class->ImplementsInterface(UPoolableInterface::StaticClass());
		DefaultObjectSettings.Class = Class;

		// Save the default actor settings
		if (Class->IsChildOf(AActor::StaticClass())) {
			AActor* DefaultActor = GetWorld()->SpawnActor(Class);
			DefaultObjectSettings.bIsActor = true;
			DefaultObjectSettings.bStartWithTickEnabled = DefaultActor->IsActorTickEnabled();
			DefaultObjectSettings.TickInterval = DefaultActor->GetActorTickInterval();
			DefaultObjectSettings.bHiddenInGame = DefaultActor->bHidden;
			DefaultObjectSettings.LifeSpan = DefaultActor->InitialLifeSpan;
			DefaultObjectSettings.bCanBeDamaged = DefaultActor->bCanBeDamaged;

			// Save the default component settings
			TArray<UActorComponent*> ActorComponents;
			DefaultActor->GetComponents<UActorComponent>(ActorComponents);
			for (int i = 0; i < ActorComponents.Num(); i++) {
				FDefaultComponentSettings DefaultComponentSettings;
				DefaultComponentSettings.bImplementsPoolableInterface = ActorComponents[i]->GetClass()->ImplementsInterface(UPoolableInterface::StaticClass());
				DefaultComponentSettings.bStartWithTickEnabled = ActorComponents[i]->IsComponentTickEnabled();
				DefaultComponentSettings.TickInterval = ActorComponents[i]->GetComponentTickInterval();
				DefaultComponentSettings.Tags = ActorComponents[i]->ComponentTags;
				DefaultComponentSettings.bAutoActivate = ActorComponents[i]->bAutoActivate;

				// Check for scene component settings
				USceneComponent* SceneComponent = Cast<USceneComponent>(ActorComponents[i]);
				if (SceneComponent->IsValidLowLevelFast()) {
					DefaultComponentSettings.bIsSceneComponent = true;
					DefaultComponentSettings.RelativeTransform = SceneComponent->GetRelativeTransform();
					DefaultComponentSettings.bIsVisible = SceneComponent->bVisible;
					DefaultComponentSettings.bIsHidden = SceneComponent->bHiddenInGame;

					// Check for static mesh component settings
					UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ActorComponents[i]);
					if (StaticMeshComponent != nullptr) {
						DefaultComponentSettings.bIsStaticMeshComponent = true;
						DefaultComponentSettings.bIsSimulatingPhysics = StaticMeshComponent->IsSimulatingPhysics();
					}
				}

				DefaultComponentsSettings.Add(DefaultComponentSettings);
			}
			DefaultActor->Destroy();

			for (int i = 0; i < NumberOfObjects; i++) {
				AActor* NewActor = GetWorld()->SpawnActor(Class);
				Add(NewActor);
			}
		}
		else {
			for (int i = 0; i < NumberOfObjects; i++) {
				Add(NewObject<UObject>((UObject*)GetTransientPackage(), Class));
			}
		}
	}

	bIsPoolHolderInitialized = true;
}

int32 APoolHolder::GetNumberOfUsedObjects() {
	return ObjectPool.Num() - AvailableObjects.Num();
}

int32 APoolHolder::GetNumberOfAvailableObjects() {
	return AvailableObjects.Num();
}

bool APoolHolder::IsObjectAvailable(UObject* Object) {
	return AvailableObjects.Contains(Object->GetName());
}

void APoolHolder::Destroyed() {
	if (DefaultObjectSettings.bIsActor) {
		TArray<UObject*> Pool;
		ObjectPool.GenerateValueArray(Pool);
		for (auto& Object : Pool) {
			AActor* Actor = Cast<AActor>(Object);
			if (!Actor->IsPendingKill()) {
				Actor->Destroy();
			}
		}
	}

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (auto& Actor : AttachedActors) {
		if (!Actor->IsPendingKill()) {
			Actor->Destroy();
		}
	}

	AvailableObjects.Empty();

	// Clear all timers
	if (DefaultObjectSettings.LifeSpan > 0) {
		TArray<FTimerHandle> TimerHandles;
		ObjectsToTimers.GenerateValueArray(TimerHandles);
		for (auto& Timer : TimerHandles) {
			GetWorldTimerManager().ClearTimer(Timer);
		}
	}

	Super::Destroyed();
}