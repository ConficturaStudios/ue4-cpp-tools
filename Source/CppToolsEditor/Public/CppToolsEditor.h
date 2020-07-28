#pragma once

#include "Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IProjectManager.h"

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

	void NewCppModule();
	void NewCppPlugin();
	void RestartEditor();

};