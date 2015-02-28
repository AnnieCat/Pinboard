// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules {

	public class DsapiPlugin : ModuleRules {

        public DsapiPlugin(TargetInfo Target)
        {
            //https://answers.unrealengine.com/questions/51798/how-can-i-enable-unwind-semantics-for-c-style-exce.html
            UEBuildConfiguration.bForceEnableExceptions = true;

            // Tell the Unreal Build Tool which Unreal modules we need
            PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
            PrivateDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore", "ShaderCore" });

            // Set up includes and .lib for DSAPI
            var path = System.Environment.GetEnvironmentVariable("DSROOT");
            PublicIncludePaths.Add(path + "\\Include");
            PublicAdditionalLibraries.Add(path + "\\Lib\\" + (Target.Platform == UnrealTargetPlatform.Win64 ? "DSAPI.lib" : "DSAPI32.lib"));

            // Set up includes and .lib for libmotion
            var libmotionPath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name)) + "\\..\\..\\..\\..\\..\\d2_gyro\\libmotion-master";
            PublicIncludePaths.Add(libmotionPath + "\\libmotiondll");
            var libPath = libmotionPath + "\\bin\\" + (Target.Platform == UnrealTargetPlatform.Win64 ? "x64" : "Win32");
            PublicAdditionalLibraries.Add(libPath + "\\libmotiondll.lib");
		}
	}
}