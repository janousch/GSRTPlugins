// Copyright 2019 (C) Ram�n Janousch

#include "GSRTUtilities.h"


UObject* UGSRTUtilities::CreateObject(TSubclassOf<UObject> Class) {
	return NewObject<UObject>((UObject*)GetTransientPackage(), Class);
}

