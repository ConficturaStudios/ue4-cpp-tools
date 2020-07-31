#pragma once

#include "Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IProjectManager.h"
#include "LevelEditor.h"
#include "Interfaces/IMainFrameModule.h"

#include "CreateModuleDialog.h"

DECLARE_LOG_CATEGORY_EXTERN(CppToolsLog, Log, All);

class CPPTOOLSEDITOR_API FCppToolsEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();

private:

	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	void FillSubmenu(FMenuBuilder& MenuBuilder);

	void OnNewCppModule();
	void RestartEditor();

    void CreateNewModule(FString Name, FCreateModuleTarget Target, EHostType::Type Type);

    FOnCreateModule OnCreateModuleDelegate;

    TSharedPtr<SWindow> CreateModuleWindow;

};