using UnrealBuildTool;
using System.IO;

public class HL2Editor : ModuleRules
{
    public HL2Editor(ReadOnlyTargetRules Target) : base(Target)
    {
		PrivatePCHHeaderFile = "Private/HL2EditorPrivatePCH.h";
		
        PrivateIncludePaths.AddRange(new string[] { "HL2Editor/Private" });
        // PublicIncludePaths.AddRange(new string[] { "HL2Editor/Public" });

        PrivateDependencyModuleNames.AddRange(new string[] { "JsonUtilities", "RHI", "Engine", "UnrealEd", "RenderCore", "Slate", "SlateCore", "EditorStyle", "DesktopPlatform", "AssetTools", "MeshDescription" });
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject" });
    }
}