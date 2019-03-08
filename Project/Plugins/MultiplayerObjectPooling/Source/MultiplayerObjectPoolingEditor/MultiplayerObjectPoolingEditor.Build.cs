namespace UnrealBuildTool.Rules
{
	public class MultiplayerObjectPoolingEditor : ModuleRules
	{
		public MultiplayerObjectPoolingEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
				new string[] {
					"MultiplayerObjectPoolingEditor/Public/Module",
					"MultiplayerObjectPoolingEditor/Public/Classes",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"MultiplayerObjectPoolingEditor/Private/Module",
					"MultiplayerObjectPoolingEditor/Private/Classes",
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Engine",
					"Core",
					"CoreUObject",
					"InputCore",
					"Slate",
					"EditorStyle",
					"AIModule",
					"BlueprintGraph",
					"MultiplayerObjectPooling"
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"EditorStyle",
					"KismetCompiler",
					"UnrealEd",
					"GraphEditor",
					"SlateCore",
                    "Kismet",
                    "KismetWidgets",
                    "PropertyEditor"
                }
			);
		}
	}
}
