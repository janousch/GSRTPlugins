
#include "K2Node_GetFromPool.h"

#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "Kismet/KismetArrayLibrary.h"
#include "ScopedTransaction.h"
#include "EdGraphUtilities.h"
#include "KismetCompiledFunctionContext.h"
#include "KismetCompilerMisc.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"

#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnumLiteral.h"
#include "UObject/UnrealType.h"
#include "Engine/EngineTypes.h"
#include "EdGraph/EdGraph.h"
#include "KismetCompiler.h"
#include "K2Node_Select.h"

#include "Engine.h"

#include "PoolManager.h"


#define LOCTEXT_NAMESPACE "ObjectPooling"


UK2Node_GetFromPool::UK2Node_GetFromPool(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to Spawn from Pool an Inactive Actor.");
}

//Helper which will store one of the function inputs we excpect BP callable function will have.
struct FK2Node_GetFromPoolHelper {
	static FString ClassPinName;
	static FString ActorPinName;
	static FString OwnerPinName;
	static FString SuccessPinName;
	static FString ReconstructPinName;
	static FString WorldContextPinName;
	static FString SpawnOptionsPinName;
	static FString NoCollisionFailPinName;
	static FString SpawnTransformPinName;
	static FString CollisionHandlingOverridePinName;
};

FString FK2Node_GetFromPoolHelper::ActorPinName(TEXT("Actor"));
FString FK2Node_GetFromPoolHelper::OwnerPinName(TEXT("Owner"));
FString FK2Node_GetFromPoolHelper::ClassPinName(TEXT("Class"));
FString FK2Node_GetFromPoolHelper::ReconstructPinName(TEXT("Reconstruct"));
FString FK2Node_GetFromPoolHelper::SuccessPinName(TEXT("SpawnSuccessful"));
FString FK2Node_GetFromPoolHelper::SpawnOptionsPinName(TEXT("SpawnOptions"));
FString FK2Node_GetFromPoolHelper::SpawnTransformPinName(TEXT("SpawnTransform"));
FString FK2Node_GetFromPoolHelper::WorldContextPinName(TEXT("WorldContextObject"));
FString FK2Node_GetFromPoolHelper::NoCollisionFailPinName(TEXT("SpawnEvenIfColliding"));
FString FK2Node_GetFromPoolHelper::CollisionHandlingOverridePinName(TEXT("CollisionHandlingOverride"));

FNodeHandlingFunctor* UK2Node_GetFromPool::CreateNodeHandler(FKismetCompilerContext &CompilerContext) const {
	return new FNodeHandlingFunctor(CompilerContext);
}

FText UK2Node_GetFromPool::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle = NSLOCTEXT("K2Node", "SpawnActorFromPool_BaseTitle", "Spawn Actor From Object-Pool ...");
	
	if (TitleType != ENodeTitleType::MenuTitle) {
		if (IsClassPinValid()) {// && ClassPin->LinkedTo.Num()>0) {
			//UEdGraphPin* ClassPin = GetClassPin();
			//auto LinkedPin = ClassPin->LinkedTo[0];
			UClass* Class = GetClassToSpawn();// (LinkedPin) ? Cast<UClass>(LinkedPin->PinType.PinSubCategoryObject.Get()) : nullptr;
			//UObjectPool* Pool = Cast<UObjectPool>(Class->ClassDefaultObject);
			//if (Pool && Pool->TemplateClass->IsValidLowLevelFast() && CachedNodeTitle.IsOutOfDate(this)) {
			if (Class && CachedNodeTitle.IsOutOfDate(this)) {
				//FText Name = Pool->TemplateClass->GetDisplayNameText();
				FText Name = Class->GetDisplayNameText();
				FFormatNamedArguments Args; Args.Add(TEXT("Name"), Name);
				CachedNodeTitle.SetCachedText(FText::Format(NSLOCTEXT("K2Node", "SpawnActorFromPool_Title", "Spawn Actor From Pool :: {Name}"), Args), this);
			}
			NodeTitle = CachedNodeTitle;
		}
	} 
	
	return NodeTitle;
}

bool UK2Node_GetFromPool::IsNodePure() const
{
	return false;
}

void UK2Node_GetFromPool::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	/// Execution Pins.
	UEdGraphPin* SpawnPin = CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	UEdGraphPin* FinishPin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	SpawnPin->PinFriendlyName = LOCTEXT("Spawn", "Spawn");
	FinishPin->PinFriendlyName = LOCTEXT("Finished", "Finished Spawning");

	/// World Context Pin.
	if (GetBlueprint()->ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin)) {
		CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UObject::StaticClass(), false, false, FK2Node_GetFromPoolHelper::WorldContextPinName);
	}///

	 /// Class Pin.
	UEdGraphPin* ClassPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, TEXT("self"), AActor::StaticClass(), false, false, FK2Node_GetFromPoolHelper::ClassPinName);

	/// Owner Pin.
	//UEdGraphPin* OwnerPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), AActor::StaticClass(), false, false, FK2Node_GetFromPoolHelper::OwnerPinName);
	//OwnerPin->bAdvancedView = true;

	/// Collision Handling Pin.
	UEnum* const MethodEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ESpawnActorCollisionHandlingMethod"), true);
	UEdGraphPin* const CollisionHandlingOverridePin = CreatePin(EGPD_Input, K2Schema->PC_Byte, TEXT(""), MethodEnum, false, false, FK2Node_GetFromPoolHelper::CollisionHandlingOverridePinName);
	CollisionHandlingOverridePin->DefaultValue = MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::Undefined));

	/// Reconstruct Pin.
	UEdGraphPin* ReconstructPin = CreatePin(EGPD_Input, K2Schema->PC_Boolean, TEXT(""), nullptr, false, false, FK2Node_GetFromPoolHelper::ReconstructPinName);

	/// Options Pin.
	//UEdGraphPin* OptionsPin = CreatePin(EGPD_Input, K2Schema->PC_Struct, TEXT(""), FPoolSpawnOptions::StaticStruct(), false, false, FK2Node_GetFromPoolHelper::SpawnOptionsPinName);

	/// Transform Pin.
	UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input, K2Schema->PC_Struct, TEXT(""), TransformStruct, false, false, FK2Node_GetFromPoolHelper::SpawnTransformPinName);

	/// Success Pin.
	UEdGraphPin* SuccessPin = CreatePin(EGPD_Output, K2Schema->PC_Boolean, TEXT(""), nullptr, false, false, FK2Node_GetFromPoolHelper::SuccessPinName);

	/// Result Pin.
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), AActor::StaticClass(), false, false, K2Schema->PN_ReturnValue);

	if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay) {
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	} 
	Super::AllocateDefaultPins();
}

UClass* UK2Node_GetFromPool::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch) const {
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* ClassPin = GetClassPin(PinsToSearch);
	UClass* SpawnClass = nullptr;

	if (IsClassPinValid()) {
		SpawnClass = Cast<UClass>(ClassPin->DefaultObject);
	}

	return SpawnClass;
}

bool UK2Node_GetFromPool::IsClassPinValid() const {
	UEdGraphPin* ClassPin = GetClassPin();
	if (ClassPin) {
		if (ClassPin->DefaultObject) {
			UClass* SpawnClass = Cast<UClass>(ClassPin->DefaultObject);
			if (SpawnClass != NULL && SpawnClass != AActor::StaticClass()) {
				return true;
			}
		}
	}

	return false;
}

void UK2Node_GetFromPool::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*> &OutClassPins) {
	check(InClass != NULL);
	
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);

	for (TFieldIterator<UProperty> PIT(InClass, EFieldIteratorFlags::IncludeSuper); PIT; ++PIT) {
		UProperty* Property = *PIT;

		const bool IsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool IsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

		if (IsExposedToSpawn && !Property->HasAnyPropertyFlags(CPF_Parm) && IsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible) && !IsDelegate && (FindPin(Property->GetName()) == NULL)) {
			UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
			const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Property, Pin->PinType);
			OutClassPins.Add(Pin);

			if (ClassDefaultObject && Pin != NULL && K2Schema->PinDefaultValueIsEditable(*Pin)) {
				FString DefaultValueAsString;
				const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString);
				check(bDefaultValueSet);
				K2Schema->TrySetDefaultValue(*Pin, DefaultValueAsString);
			} 
			
			if (Pin != nullptr) {
				K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip); 
			}
		}
	}

	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass;
}

void UK2Node_GetFromPool::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*> &OldPins) {
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if (UseSpawnClass != NULL)
	{
		TArray<UEdGraphPin*> ClassPins;
		CreatePinsForClass(UseSpawnClass, ClassPins);
	}

	RestoreSplitPins(OldPins);
}

bool UK2Node_GetFromPool::IsSpawnVarPin(UEdGraphPin* Pin) {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* ParentPin = Pin->ParentPin;
	while (ParentPin) {
		if (ParentPin->PinName == FK2Node_GetFromPoolHelper::SpawnTransformPinName) { return false; }
		ParentPin = ParentPin->ParentPin;
	}

	return (
		Pin->PinName != K2Schema->PN_Execute &&
		Pin->PinName != K2Schema->PN_Then &&
		Pin->PinName != K2Schema->PN_ReturnValue &&
		Pin->PinName != FK2Node_GetFromPoolHelper::ClassPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::OwnerPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::SuccessPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::ReconstructPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::WorldContextPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::SpawnOptionsPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::SpawnTransformPinName &&
		Pin->PinName != FK2Node_GetFromPoolHelper::CollisionHandlingOverridePinName
		);
}

void UK2Node_GetFromPool::PostPlacedNewNode() {
	Super::PostPlacedNewNode();
	UClass* UseSpawnClass = GetClassToSpawn();
	if (UseSpawnClass != NULL)
	{
		TArray<UEdGraphPin*> ClassPins;
		CreatePinsForClass(UseSpawnClass, ClassPins);
	}
}

void UK2Node_GetFromPool::OnClassPinChanged() {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	TArray<UEdGraphPin*> OldPins = Pins;
	TArray<UEdGraphPin*> OldClassPins;
	for (int32 i = 0; i < OldPins.Num(); i++)
	{
		UEdGraphPin* OldPin = OldPins[i];
		if (IsSpawnVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}
	CachedNodeTitle.MarkDirty();
	UClass* UseSpawnClass = GetClassToSpawn();
	TArray<UEdGraphPin*> NewClassPins;
	if (UseSpawnClass != NULL)
	{
		CreatePinsForClass(UseSpawnClass, NewClassPins);
	}
	UEdGraphPin* ResultPin = GetResultPin();
	TArray<UEdGraphPin*> ResultPinConnectionList = ResultPin->LinkedTo;
	ResultPin->BreakAllPinLinks();
	for (UEdGraphPin* Connections : ResultPinConnectionList)
	{
		K2Schema->TryCreateConnection(ResultPin, Connections);
	}
	RewireOldPinsToNewPins(OldClassPins, NewClassPins);
	DestroyPinList(OldClassPins);
	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_GetFromPool::PinConnectionListChanged(UEdGraphPin* ChangedPin) {
	if (ChangedPin && (ChangedPin->PinName == FK2Node_GetFromPoolHelper::ClassPinName)) {
		OnClassPinChanged(); 
	}
	
}

void UK2Node_GetFromPool::PinDefaultValueChanged(UEdGraphPin* ChangedPin) {
	if (ChangedPin && (ChangedPin->PinName == FK2Node_GetFromPoolHelper::ClassPinName)) { 
		OnClassPinChanged(); 
	}
}

void UK2Node_GetFromPool::ExpandNode(class FKismetCompilerContext &CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext, SourceGraph);

	static FName BeginSpawningBlueprintFuncName = GET_FUNCTION_NAME_CHECKED(APoolManager, BeginDeferredSpawnFromPool);
	static FName FinishSpawningFuncName = GET_FUNCTION_NAME_CHECKED(APoolManager, FinishDeferredSpawnFromPool);

	UK2Node_GetFromPool* SpawnNode = this;
	UEdGraphPin* SpawnClassPin = SpawnNode->GetClassPin();
	UEdGraphPin* SpawnNodeExec = SpawnNode->GetExecPin();
	UEdGraphPin* SpawnNodeThen = SpawnNode->GetThenPin();
	UEdGraphPin* SpawnNodeResult = SpawnNode->GetResultPin();
	UEdGraphPin* SpawnSuccessPin = SpawnNode->GetSuccessPin();
	//UEdGraphPin* SpawnNodeOwnerPin = SpawnNode->GetOwnerPin();
	//UEdGraphPin* SpawnNodeOptions = SpawnNode->GetSpawnOptionsPin();
	UEdGraphPin* SpawnWorldContextPin = SpawnNode->GetWorldContextPin();
	UEdGraphPin* SpawnNodeTransform = SpawnNode->GetSpawnTransformPin();
	UEdGraphPin* SpawnNodeReconstructPin = SpawnNode->GetReconstructPin();
	UEdGraphPin* SpawnNodeCollisionHandlingOverride = GetCollisionHandlingOverridePin();

	UClass* ClassToSpawn = GetClassToSpawn();
	if (!SpawnClassPin) {
		CompilerContext.MessageLog.Error(*LOCTEXT("SpawnActorFromPool_Pool_Error", "Spawn Node: @@ must have a @@ specified!").ToString(), SpawnNode, SpawnClassPin);
		SpawnNode->BreakAllNodeLinks();
		return;
	}


	UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallBeginSpawnNode->FunctionReference.SetExternalMember(BeginSpawningBlueprintFuncName, APoolManager::StaticClass());
	CallBeginSpawnNode->AllocateDefaultPins();

	UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
	UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();
	//UEdGraphPin* CallBeginOwnerPin = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::OwnerPinName);
	UEdGraphPin* CallBeginActorClassPin = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::ClassPinName);
	UEdGraphPin* CallBeginSuccessPin = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::SuccessPinName);
	//UEdGraphPin* CallBeginOptions = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::SpawnOptionsPinName);
	UEdGraphPin* CallBeginTransform = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::SpawnTransformPinName);
	UEdGraphPin* CallBeginReconstructPin = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::ReconstructPinName);
	UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::WorldContextPinName);
	UEdGraphPin* CallBeginCollisionHandlingOverride = CallBeginSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::CollisionHandlingOverridePinName);

	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallBeginExec);
	CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallBeginActorClassPin);
	//CompilerContext.MovePinLinksToIntermediate(*SpawnNodeOptions, *CallBeginOptions);
	CompilerContext.MovePinLinksToIntermediate(*SpawnSuccessPin, *CallBeginSuccessPin);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeTransform, *CallBeginTransform);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeReconstructPin, *CallBeginReconstructPin);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeCollisionHandlingOverride, *CallBeginCollisionHandlingOverride);

	if (SpawnWorldContextPin) { CompilerContext.MovePinLinksToIntermediate(*SpawnWorldContextPin, *CallBeginWorldContextPin); }
	//if (SpawnNodeOwnerPin != nullptr) { CompilerContext.MovePinLinksToIntermediate(*SpawnNodeOwnerPin, *CallBeginOwnerPin); }


	UK2Node_CallFunction* CallFinishSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode, SourceGraph);
	CallFinishSpawnNode->FunctionReference.SetExternalMember(FinishSpawningFuncName, APoolManager::StaticClass());
	CallFinishSpawnNode->AllocateDefaultPins();

	UEdGraphPin* CallFinishExec = CallFinishSpawnNode->GetExecPin();
	UEdGraphPin* CallFinishThen = CallFinishSpawnNode->GetThenPin();
	UEdGraphPin* CallFinishResult = CallFinishSpawnNode->GetReturnValuePin();
	//UEdGraphPin* CallFinishClass = CallFinishSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::ClassPinName);
	UEdGraphPin* CallFinishActor = CallFinishSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::ActorPinName);
	UEdGraphPin* CallFinishTransform = CallFinishSpawnNode->FindPinChecked(FK2Node_GetFromPoolHelper::SpawnTransformPinName);

	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *CallFinishThen);
	CompilerContext.CopyPinLinksToIntermediate(*CallBeginTransform, *CallFinishTransform);

	CallBeginResult->MakeLinkTo(CallFinishActor);
	CallFinishResult->PinType = SpawnNodeResult->PinType;
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallFinishResult);


	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallBeginSpawnNode, SpawnNode, CallBeginResult, ClassToSpawn);
	LastThen->MakeLinkTo(CallFinishExec);

	SpawnNode->BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_GetFromPool::GetThenPin() const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction == EGPD_Output); return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetClassPin(const TArray<UEdGraphPin*>* InPinsToSearch) const {
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;

	UEdGraphPin* Pin = NULL;
	for (auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* TestPin = *PinIt;
		if (TestPin && TestPin->PinName == FK2Node_GetFromPoolHelper::ClassPinName)
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetSpawnOptionsPin() const {
	UEdGraphPin* Pin = FindPinChecked(FK2Node_GetFromPoolHelper::SpawnOptionsPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetSpawnTransformPin() const {
	UEdGraphPin* Pin = FindPinChecked(FK2Node_GetFromPoolHelper::SpawnTransformPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetCollisionHandlingOverridePin() const {
	UEdGraphPin* const Pin = FindPinChecked(FK2Node_GetFromPoolHelper::CollisionHandlingOverridePinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetOwnerPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_GetFromPoolHelper::OwnerPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetReconstructPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_GetFromPoolHelper::ReconstructPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input); return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetWorldContextPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_GetFromPoolHelper::WorldContextPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetSuccessPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_GetFromPoolHelper::SuccessPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_GetFromPool::GetResultPin() const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FLinearColor UK2Node_GetFromPool::GetNodeTitleColor() const {
	//FColor Color = FColor(75.f, 209.f, 246.f); // Lovely Pool Water
	FDateTime Now = FDateTime::Now();
	float Second = Now.GetSecond();
	float Minute = Now.GetMinute();
	float Hour = Now.GetHour();
	//FColor Color = FColor::MakeRandomColor();
	FColor Color = FColor::White;
	return FLinearColor(Color);
}

bool UK2Node_GetFromPool::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const {
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || (FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph && Blueprint->GeneratedClass->GetDefaultObject()->ImplementsGetWorld()));
}

void UK2Node_GetFromPool::GetNodeAttributes(TArray<TKeyValuePair<FString, FString>> &OutNodeAttributes) const {
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT("InvalidClass");
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Type"), TEXT("SpawnActorFromPool")));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Class"), GetClass()->GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("Name"), GetName()));
	OutNodeAttributes.Add(TKeyValuePair<FString, FString>(TEXT("ActorClass"), ClassToSpawnStr));
}

bool UK2Node_GetFromPool::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const {
	UClass* SourceClass = GetClassToSpawn();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
	if (bResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass);
	}
	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bSuperResult || bResult;
}

void UK2Node_GetFromPool::GetPinHoverText(const UEdGraphPin &Pin, FString &HoverTextOut) const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	//
	if (UEdGraphPin* ClassPin = GetClassPin()) {
		K2Schema->ConstructBasicPinTooltip(*ClassPin, LOCTEXT("ClassPinDescription", "The Object-Pool Owner of Actor Instance."), ClassPin->PinToolTip);
	} if (UEdGraphPin* TransformPin = GetSpawnTransformPin()) {
		K2Schema->ConstructBasicPinTooltip(*TransformPin, LOCTEXT("TransformPinDescription", "The transform to spawn the Actor with"), TransformPin->PinToolTip);
	} if (UEdGraphPin* CollisionHandlingOverridePin = GetCollisionHandlingOverridePin()) {
		K2Schema->ConstructBasicPinTooltip(*CollisionHandlingOverridePin, LOCTEXT("CollisionHandlingOverridePinDescription", "Specifies how to handle collisions at the Spawn Point. If undefined,uses Actor Class Settings."), CollisionHandlingOverridePin->PinToolTip);
	} if (UEdGraphPin* OwnerPin = GetOwnerPin()) {
		K2Schema->ConstructBasicPinTooltip(*OwnerPin, LOCTEXT("OwnerPinDescription", "Can be left empty; primarily used for replication or visibility."), OwnerPin->PinToolTip);
	} if (UEdGraphPin* ReconstructPin = GetReconstructPin()) {
		K2Schema->ConstructBasicPinTooltip(*ReconstructPin, LOCTEXT("ReconstructPinDescription", "If checked, this will force the Spawning Actor to re-run its Construction Scripts;\nThis results on expensive respawn operation and will affect performance!\nAvoid pulling Actors every second with this option enabled."), ReconstructPin->PinToolTip);
	} if (UEdGraphPin* ResultPin = GetResultPin()) {
		K2Schema->ConstructBasicPinTooltip(*ResultPin, LOCTEXT("ResultPinDescription", "The Actor Class Spawned from the Pool."), ResultPin->PinToolTip);
	} return Super::GetPinHoverText(Pin, HoverTextOut);
}

FText UK2Node_GetFromPool::GetTooltipText() const {
	return NodeTooltip;
}

/*
FSlateIcon UK2Node_GetFromPool::GetIconAndTint(FLinearColor &OutColor) const {
	static FSlateIcon Icon(TEXT("OBJPoolStyle"), TEXT("ClassIcon.ObjectPool"));
	return Icon;
}
*/

bool UK2Node_GetFromPool::ArePinTypesEqual(const FEdGraphPinType& A, const FEdGraphPinType& B)
{
	if(A.PinCategory != B.PinCategory)
	{
		return false;
	}
	if(A.PinSubCategory != B.PinSubCategory)
	{
		return false;
	}
	if(A.PinSubCategoryObject != B.PinSubCategoryObject)
	{
		return false;
	}
	return true;
}


bool UK2Node_GetFromPool::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	auto Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if(!ensure(Schema) || (ensure(OtherPin) && Schema->IsExecPin(*OtherPin) && !Schema->IsExecPin(*MyPin)))
	{
		OutReason = NSLOCTEXT("K2Node", "MakeArray_InputIsExec", "Doesn't allow execution input!").ToString();
		return true;
	}

	return false;
}

void UK2Node_GetFromPool::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if(ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_GetFromPool::GetMenuCategory() const
{
	return FText::FromString("Object Pooling|Multiplayer");
}

#undef LOCTEXT_NAMESPACE
