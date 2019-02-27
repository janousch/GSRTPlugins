// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSRTUtilities.generated.h"

/**
 * 
 */
UCLASS()
class UGSRTUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	
public:

	UFUNCTION(BlueprintPure, Category = "GSRT", Meta = (ToolTip = "Create a single object", DeterminesOutputType = "Class", Keywords = "Create Object"))
		static UObject* CreateObject(TSubclassOf<UObject> Class);
	
};
