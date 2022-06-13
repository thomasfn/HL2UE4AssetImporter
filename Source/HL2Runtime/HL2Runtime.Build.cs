using UnrealBuildTool;
using System.IO;

public class HL2Runtime : ModuleRules
{
    public HL2Runtime(ReadOnlyTargetRules Target) : base(Target)
    {
		PrivatePCHHeaderFile = "Private/HL2RuntimePrivatePCH.h";
		
        PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

        PrivateDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore", "PhysicsCore" });
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
    }
}