// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Interfaces/IPluginManager.h"

#include "DesktopPlatformModule.h"

#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/FeedbackContext.h"

#include "GeneralProjectSettings.h"
#include "SourceCodeNavigation.h"
#include "ProjectDescriptor.h"

#include "GameProjectUtils.h"

#include "Widgets/Notifications/SNotificationList.h"

/**
 * 
 */
class CPPTOOLSEDITOR_API CppToolsUtil
{
public:

    /** Gets the Content directory of the CppTools Plugin. */
    static FString CppToolsContentDir();

    /** Reads in a custom template file from the CppTools Content folder. */
    static bool ReadCustomTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason);

    static FString GetCopyrightLine();
    static FString GetModuleAPIMacro(const FString& ModuleName, bool bIsPrivate);

    static bool GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames, const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs);
    static bool GenerateModuleHeaderFile(const FString& NewHeaderFileName, const FString& ModuleName, const TArray<FString>& PublicHeaderIncludes, FText& OutFailReason);
    static bool GenerateModuleCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& StartupSourceCode, const FString& ShutdownSourceCode, FText& OutFailReason);

    static GameProjectUtils::EAddCodeToProjectResult GenerateModule(const FString& ModulePath, const FString& ModuleName, const EHostType::Type& Type, bool bUsePCH, TArray<FString>& CreatedFiles, FText& OutFailReason);

private:

    static bool UpdateGameProjectFile(const FString& ProjectFile, const FString& EngineIdentifier, const FProjectDescriptorModifier* Modifier, FText& OutFailReason);
    static bool UpdateGameProject(const FProjectDescriptorModifier* Modifier);

    static bool AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules);

};
