using UnrealBuildTool;
using System;
using System.IO;

public class CerealUE4 : ModuleRules
{
	public CerealUE4(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		switch (Target.Configuration)
		{
			case UnrealTargetConfiguration.Debug:
			case UnrealTargetConfiguration.DebugGame:
				break;

			case UnrealTargetConfiguration.Development:
			case UnrealTargetConfiguration.Shipping:
				PublicDefinitions.Add("CEREAL_NO_NOEXCEPT");
				break;

			default:
				throw new Exception(String.Format("Build configuration is not supported yet: {0}", Target.Configuration));
		}

		PublicIncludePaths.AddRange(
			new string[] {
				ModuleDirectory,
				ModuleDirectory+"/cereal-UE4/include",
				ModuleDirectory+"/cereal/include"
				}
			);
		System.Console.WriteLine(" CerealUE4:" + ModuleDirectory);
	}
}
