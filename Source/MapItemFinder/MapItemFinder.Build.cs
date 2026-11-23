using UnrealBuildTool;

public class MapItemFinder : ModuleRules
{
	public MapItemFinder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"FactoryGame",
			"SML"
		});

		if (Target.Type != TargetType.Server)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Slate",
				"SlateCore",
				"InputCore",
				"UMG"
			});
		}
	}
}
