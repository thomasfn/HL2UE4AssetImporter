using UnrealBuildTool;
using System.IO;

public class VTFLib : ModuleRules
{
	protected virtual string IncDirectory { get { return Path.Combine(ModuleDirectory, "include"); } }

	protected virtual string LibDirectory { get { return Path.Combine(ModuleDirectory, "lib"); } }

	protected virtual string BinDirectory { get { return Path.Combine(ModuleDirectory, "bin"); } }

	public VTFLib(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        PublicDefinitions.Add("WITH_VTFLIB=1");

        PublicIncludePaths.Add(IncDirectory);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibDirectory, "Win64", "VTFLib.lib"));
			RuntimeDependencies.Add(Path.Combine(BinDirectory, "Win64", "VTFLib.dll"));
			PublicDelayLoadDLLs.Add("VTFLib.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibDirectory, "Win32", "VTFLib.lib"));
			RuntimeDependencies.Add(Path.Combine(BinDirectory, "Win32", "VTFLib.dll"));
			PublicDelayLoadDLLs.Add("VTFLib.dll");
		}
		// TODO: Add linux/mac binaries
    }
}