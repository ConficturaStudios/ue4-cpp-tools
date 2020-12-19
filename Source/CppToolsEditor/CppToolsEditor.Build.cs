using UnrealBuildTool;
using System.IO;

public class CppToolsEditor : ModuleRules
{
    public CppToolsEditor(ReadOnlyTargetRules Target) : base(Target)
    {

        PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });
        PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AssetTools", "ApplicationCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UnrealEd", "GameProjectGeneration", "LevelEditor",
            "Projects", "MainFrame", "AppFramework", "EditorStyle" , "EngineSettings", "SourceControl", "DesktopPlatform" });
      
    }
}