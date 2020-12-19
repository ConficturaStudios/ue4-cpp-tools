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

#include "Framework/Notifications/NotificationManager.h"

#include "GeneralProjectSettings.h"
#include "SourceCodeNavigation.h"
#include "ProjectDescriptor.h"

#include "GameProjectUtils.h"

#include "Widgets/Notifications/SNotificationList.h"

DECLARE_DELEGATE_RetVal_OneParam(bool, FPluginDescriptorModifier, FPluginDescriptor&);

/**
 * A class used to contain static utility functions for extending editor functionality for programmers.
 */
class CPPTOOLSEDITOR_API CppToolsUtil
{
public:

    // --- File utilities ---

    /** Gets the Content directory of the CppTools Plugin. */
    static FString CppToolsContentDir();

    /** Checks out the specified file from source control. */
    static bool CheckoutFile(const FString& Filename, FText& OutFailReason);
    /** Attempts to check out the .uplugin file and make it writable if it is not already. */
    static void TryMakePluginFileWriteable(const FString& PluginFile);

    /** Reads in a custom template file from the CppTools Content folder. */
    static bool ReadCustomTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason);

    /** Finds the specified file within the project. The found path is retrieved through the OutPath parameter. */
    static bool FindFileInProject(const FString& InFilename, const FString& InSearchPath, FString& OutPath);

    // --- Text processing ---

    /** Removes C Style comments from the provided string and returns the result. */
    static FString StripCStyleComments(const FString& SourceString);
    /** Parses a string containing a list of strings into an array of individual strings. */
    static TArray<FString> ParseStringList(const FString& RawStringList);
    /** Creates a single comma separated list of strings from an array of strings. */
    static FString CombineStringList(const TArray<FString>& StringList, bool bUseQuotes, bool bIncludeSpace);
    
    // --- Editor utilities ---
    
    /** Pushes a new notification to the editor. */
    static void PushNotification(const FText& Text, const SNotificationItem::ECompletionState& CompletionState);

    // --- Templating getters ---
    
    /** Gets the copyright notice. */
    static FString GetCopyrightLine();
    /** Gets the API macro for the specified module. */
    static FString GetModuleAPIMacro(const FString& ModuleName, bool bIsPrivate);

    // --- Module and plugin utilities ---

    /**
     * Gets the .Build.cs file for the specified module in the provided plugin. Use NULL for the plugin
     * if you want to access a module from the game project.
     */
    static bool GetModuleBuildFilePath(const FString& ModuleName, TSharedPtr<IPlugin> Target, FString& OutBuildFilePath);

    /** Gets all of the plugins with their source contained in this project. */
    static TArray<TSharedPtr<IPlugin>> GetProjectPlugins();

    /** Gets the context information for every module in the game project. */
    static TArray<FModuleContextInfo> GetProjectModules();
    /**
     * Gets the context information for every module in the specified plugin.
     * The specified plugin must be contained within this project.
     */
    static TArray<FModuleContextInfo> GetPluginModules(const TSharedPtr<IPlugin>& Plugin);

    /** Gets the list of public module dependencies of the specified module. */
    static TArray<FString> GetModuleDependencies(const FString& ModuleName, TSharedPtr<IPlugin> Target, bool bIncludePrivate);

    // --- File generation functions ---
    
    static bool GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames,
        const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs);
    static bool GenerateModuleHeaderFile(const FString& NewHeaderFileName, const FString& ModuleName, const TArray<FString>& PublicHeaderIncludes,
        FText& OutFailReason);
    static bool GenerateModuleCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& StartupSourceCode,
        const FString& ShutdownSourceCode, FText& OutFailReason);

    // --- File modification functions ---

    static bool InsertDependencyIntoModule(const FString& ModuleName, TSharedPtr<IPlugin> Target, const FString& DependencyName, FText& OutFailReason, bool bPrivate);
    static bool InsertDependencyIntoTarget(const FString& ModuleName, const bool& bIsEditor, FText& OutFailReason);

    // --- Primary functionality ---
    
    static GameProjectUtils::EAddCodeToProjectResult GenerateModule(const FString& ModulePath, TSharedPtr<IPlugin> Target,
        const FString& ModuleName, const EHostType::Type& Type, const ELoadingPhase::Type& LoadingPhase, bool bUsePCH,
        TArray<FString>& CreatedFiles, FText& OutFailReason);
    
private:

    // --- Manage UProject ---

    static bool UpdateGameProjectFile(const FString& ProjectFile, const FString& EngineIdentifier, const FProjectDescriptorModifier* Modifier, FText& OutFailReason);
    static bool UpdateGameProject(const FProjectDescriptorModifier* Modifier);

    static bool AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules);

    // --- Manage UPlugin ---
    
    static bool UpdatePluginFile(const FString& PluginFile, const FPluginDescriptorModifier* Modifier, FText& OutFailReason);
    static bool UpdatePlugin(const TSharedPtr<IPlugin>&, const FPluginDescriptorModifier* Modifier);

    static bool AppendPluginModules(FPluginDescriptor& Descriptor, TSharedPtr<IPlugin> Plugin, const TArray<FModuleDescriptor>* Modules);

};
