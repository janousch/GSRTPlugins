// Copyright 2019 (C) Ramón Janousch

#include "GSRTUtilities.h"


UObject* UGSRTUtilities::CreateObject(TSubclassOf<UObject> Class) {
	return NewObject<UObject>((UObject*)GetTransientPackage(), Class);
}

