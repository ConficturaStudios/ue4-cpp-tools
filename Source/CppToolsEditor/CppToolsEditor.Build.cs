using UnrealBuildTool;
using System.IO;

public class CppToolsEditor : ModuleRules
{
    public CppToolsEditor(ReadOnlyTargetRules Target) : base(Target)
    {

        PrivateIncludePaths.AddRange(new string[] { "CppToolsEditor/Private" });
        PublicIncludePaths.AddRange(new string[] { "CppToolsEditor/Public" });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AssetTools", "ApplicationCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UnrealEd", "GameProjectGeneration", "LevelEditor",
            "Projects", "MainFrame", "AppFramework", "EditorStyle" , "EngineSettings", "SourceControl", "DesktopPlatform" });
      
    }
}