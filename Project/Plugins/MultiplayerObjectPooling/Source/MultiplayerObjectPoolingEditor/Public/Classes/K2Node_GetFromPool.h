#pragma once

#include "CoreMinimal.h"

#include "K2Node.h"

#include "Textures/SlateIcon.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MINOR_VERSION > 12
	#include "Textures/SlateIcon.h"
#endif

#include "PoolManager.h"

#include "K2Node_GetFromPool.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;
class APoolManager;

UCLASS()
class UK2Node_GetFromPool : public UK2Node
{
	GENERATED_UCLASS_BODY()

public:

	/** Get the owning player pin */
	//UEdGraphPin* GetOwningPlayerPin() const;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void AllocateDefaultPins() override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FText GetTooltipText() const override;
	//virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;

	virtual void PostPlacedNewNode() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void GetPinHoverText(const UEdGraphPin &Pin, FString &HoverTextOut) const override;
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;

	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*> &OldPins) override;
	virtual void GetNodeAttributes(TArray<TKeyValuePair<FString, FString>> &OutNodeAttributes) const override;

	bool IsSpawnVarPin(UEdGraphPin* Pin);
	void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*> &OutClassPins);
	bool IsClassPinValid() const;

	UEdGraphPin* GetThenPin() const;
	UEdGraphPin* GetResultPin() const;
	UEdGraphPin* GetOwnerPin() const;
	UEdGraphPin* GetSuccessPin() const;
	UEdGraphPin* GetReconstructPin() const;
	UEdGraphPin* GetWorldContextPin() const;
	UEdGraphPin* GetSpawnOptionsPin() const;
	UEdGraphPin* GetSpawnTransformPin() const;
	UEdGraphPin* GetCollisionHandlingOverridePin() const;
	UEdGraphPin* GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch = nullptr) const;
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch = nullptr) const;

#if ENGINE_MINOR_VERSION <= 12
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override
	{
		return TEXT("Kismet.AllClasses.FunctionIcon");
	}
#else
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override
	{
		static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
		return Icon;
	}
#endif
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const override;
	//virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }
	// End of UK2Node interface


protected:
	friend class FKismetCompilerContext;

	/** Propagates the pin type from the output (array) pin to the inputs, to make sure types are consistent */
	//void PropagatePinType();

	bool ArePinTypesEqual(const FEdGraphPinType& A, const FEdGraphPinType& B);

	/** Gets base class to use for the 'class' pin.  UObject by default. */
	//virtual UClass* GetClassPinBaseClass() const;

	FText NodeTooltip;

	FNodeTextCache CachedNodeTitle;

	void OnClassPinChanged();

};
