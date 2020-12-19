#include "CppToolsEditor.h"
#include "CppToolsEditorPrivatePCH.h"

//#include "Developer/AssetTools/Public/IAssetTools.h"
//#include "Developer/AssetTools/Public/AssetToolsModule.h"


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
        MenuBuilder.AddMenuEntry(
            FText::FromString("New C++ Module..."),
            FText::FromString("Create a new C++ Module"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FCppToolsEditorModule::OnNewCppModule))
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

void FCppToolsEditorModule::OnNewCppModule() {
    UE_LOG(CppToolsLog, Log, TEXT("Test add new cpp module!"));
    CreateModuleWindow = SNew(SWindow)
        .Title(NSLOCTEXT("CreateNewModule", "WindowTitle", "Create New C++ Module"))
        .ClientSize(FVector2D(750, 350))
        .SizingRule(ESizingRule::FixedSize)
        .SupportsMinimize(false).SupportsMaximize(false);

    OnCreateModuleDelegate.BindRaw(this, &FCppToolsEditorModule::CreateNewModule);

    TSharedRef<SCreateModuleDialog> CreateModuleDialog = SNew(SCreateModuleDialog).OnCreateModule(OnCreateModuleDelegate);
    CreateModuleWindow->SetContent(CreateModuleDialog);

    TSharedPtr<SWindow> ParentWindow;
    if (FModuleManager::Get().IsModuleLoaded("MainFrame")) {
        IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame");
        ParentWindow = MainFrame.GetParentWindow();
    }

    bool modal = false;
    if (modal) {
        FSlateApplication::Get().AddModalWindow(CreateModuleWindow.ToSharedRef(), ParentWindow);
    }
    else if (ParentWindow.IsValid()) {
        FSlateApplication::Get().AddWindowAsNativeChild(CreateModuleWindow.ToSharedRef(), ParentWindow.ToSharedRef());
    }
    else {
        FSlateApplication::Get().AddWindow(CreateModuleWindow.ToSharedRef());
    }
    CreateModuleWindow->ShowWindow();

}

void FCppToolsEditorModule::RestartEditor() {
    UE_LOG(CppToolsLog, Log, TEXT("Restarting editor..."));
    FUnrealEdMisc::Get().RestartEditor(false);
}

void FCppToolsEditorModule::CreateNewModule(FString Name, FCreateModuleTarget Target, EHostType::Type Type) {
    UE_LOG(CppToolsLog, Log, TEXT("Creating module..."));
}

IMPLEMENT_MODULE(FCppToolsEditorModule, CppToolsEditor)

#undef LOCTEXT_NAMESPACE