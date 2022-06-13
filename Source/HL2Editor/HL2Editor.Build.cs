using UnrealBuildTool;
using System.IO;

public class HL2Editor : ModuleRules
{
    public HL2Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "JsonUtilities", "Engine",
            "RenderCore", "RHI",
            "UnrealEd", "Slate", "SlateCore", "EditorStyle", "DesktopPlatform", "AssetTools", "ContentBrowser", "EditorScriptingUtilities", "Projects", "AssetRegistry",
            "MeshDescription", "StaticMeshDescription", "MeshUtilitiesCommon", "MeshDescriptionOperations",
            "Landscape", "Foliage", "BSPUtils",
            "HL2Runtime", "VTFLib"
        });
    }
}