// Copyright 2019 (C) Ramón Janousch

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSRTUtilities.generated.h"

DECLARE_DYNAMIC_DELEGATE(FEventName);

/**
 * 
 */
UCLASS()
class UGSRTUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	
public:

	UFUNCTION(BlueprintPure, Category = "GSRT|Helper", Meta = (ToolTip = "Create a single object", DeterminesOutputType = "Class", Keywords = "Create Object"))
		static UObject* CreateObject(TSubclassOf<UObject> Class);

	UFUNCTION(BlueprintPure, Category = "GSRT|Helper", Meta = (ToolTip = "Get the name of the object", DefaultToSelf = "Object", Keywords = "Object Pool", DisplayName = "GetName"))
		static FString GetObjectName(UObject* Object);

	UFUNCTION(BlueprintPure, Category = "GSRT|Helper", Meta = (ToolTip = "Get the Event name of the delegate"))
		static FString GetDelegateName(FEventName Event);

	UFUNCTION(BlueprintPure, Category = "GSRT|Helper", Meta = (ToolTip = "Get the object name of the delegate (not the class name!)"))
		static FString GetDelegateObjectName(FEventName Event);
};
