#include "CppToolsEditor.h"
#include "CppToolsEditorPrivatePCH.h"

//#include "Developer/AssetTools/Public/IAssetTools.h"
//#include "Developer/AssetTools/Public/AssetToolsModule.h"

#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "CppToolsEditor"

DEFINE_LOG_CATEGORY(CppToolsLog)

void FCppToolsEditorModule::StartupModule()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuExtension(
        "FileProject",
        EExtensionHook::After,
        NULL,
        FMenuExtensionDelegate::CreateRaw(this, &FCppToolsEditorModule::AddMenuEntry)
        );
    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FCppToolsEditorModule::ShutdownModule()
{
}

void FCppToolsEditorModule::AddMenuEntry(FMenuBuilder& MenuBuilder) {
    MenuBuilder.BeginSection("CppTools", TAttribute<FText>(FText::FromString("C++ Tools")));
    {
        /*MenuBuilder.AddSubMenu(FText::FromString("Cpp Tools Submenu"),
            FText::FromString("Cpp Tools Submenu Tooltip"),
            FNewMenuDelegate::CreateRaw(this, &FCppToolsEditorModule::FillSubmenu));*/
        MenuBuilder.AddMenuEntry(
            FText::FromString("New C++ Module..."),
            FText::FromString("Create a new C++ Module"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FCppToolsEditorModule::NewCppModule))
        );
        MenuBuilder.AddMenuEntry(
            FText::FromString("Restart Editor"),
            FText::FromString("Restarts the UE4 Editor"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FCppToolsEditorModule::RestartEditor))
        );
    }
    MenuBuilder.EndSection();
}

void FCppToolsEditorModule::FillSubmenu(FMenuBuilder& MenuBuilder) {
    MenuBuilder.AddMenuEntry(
        FText::FromString("Restart Editor"),
        FText::FromString("Restart Editor Tooltip"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FCppToolsEditorModule::RestartEditor))
    );
    MenuBuilder.AddMenuEntry(
        FText::FromString("New Cpp Module..."),
        FText::FromString("New Cpp Module Tooltip"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FCppToolsEditorModule::NewCppModule))
    );
}

void FCppToolsEditorModule::NewCppModule() {
    UE_LOG(CppToolsLog, Log, TEXT("Test add new cpp module!"));
}

void FCppToolsEditorModule::NewCppPlugin() {
    UE_LOG(CppToolsLog, Log, TEXT("Test add new cpp plugin!"));
}

void FCppToolsEditorModule::RestartEditor() {
    UE_LOG(CppToolsLog, Log, TEXT("Restarting editor..."));
    FUnrealEdMisc::Get().RestartEditor(false);
}

IMPLEMENT_MODULE(FCppToolsEditorModule, CppToolsEditor)

#undef LOCTEXT_NAMESPACE