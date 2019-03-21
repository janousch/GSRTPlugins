// Copyright 2019 (C) Ramon Janousch

#include "GSRTUtilities.h"


UObject* UGSRTUtilities::CreateObject(UClass* Class) {
	return NewObject<UObject>((UObject*)GetTransientPackage(), Class);
}

FString UGSRTUtilities::GetObjectName(UObject* Object) {
	if (!Object->IsValidLowLevel()) return "None";

	FString FullName = Object->GetFullName();
	FString Path;
	FString Name;
	FullName.Split(":PersistentLevel.", &Path, &Name);

	return Name;
}

FString UGSRTUtilities::GetDelegateName(FEventName Event) {
	return Event.GetFunctionName().ToString();
}

FString UGSRTUtilities::GetDelegateObjectName(FEventName Event) {
	
	return GetObjectName(Event.GetUObject());
}