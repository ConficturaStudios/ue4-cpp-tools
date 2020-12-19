// Fill out your copyright notice in the Description page of Project Settings.


#include "CppToolsUtil.h"

#include "Editor/EditorPerProjectUserSettings.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "CppToolsUtil"


FString CppToolsUtil::CppToolsContentDir() {
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("CppTools"))->GetContentDir();
    return ContentDir;
}

bool CppToolsUtil::CheckoutFile(const FString& Filename, FText& OutFailReason)
{
    // See GameProjectUtils.cpp L3700
    
    if ( !ensure(Filename.Len()) )
    {
        OutFailReason = LOCTEXT("NoFilename", "The filename was not specified.");
        return false;
    }

    if ( !ISourceControlModule::Get().IsEnabled() )
    {
        OutFailReason = LOCTEXT("SCCDisabled", "Source control is not enabled. Enable source control in the preferences menu.");
        return false;
    }

    FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(Filename);
    ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
    FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
    TArray<FString> FilesToBeCheckedOut;
    FilesToBeCheckedOut.Add(AbsoluteFilename);

    bool bSuccessfullyCheckedOut = false;
    OutFailReason = LOCTEXT("SCCStateInvalid", "Could not determine source control state.");

    if(SourceControlState.IsValid())
    {
        if(SourceControlState->IsCheckedOut() || SourceControlState->IsAdded() || !SourceControlState->IsSourceControlled())
        {
            // Already checked out or opened for add... or not in the depot at all
            bSuccessfullyCheckedOut = true;
        }
        else if(SourceControlState->CanCheckout() || SourceControlState->IsCheckedOutOther())
        {
            bSuccessfullyCheckedOut = (SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToBeCheckedOut) == ECommandResult::Succeeded);
            if (!bSuccessfullyCheckedOut)
            {
                OutFailReason = LOCTEXT("SCCCheckoutFailed", "Failed to check out the project file.");
            }
        }
        else if(!SourceControlState->IsCurrent())
        {
            OutFailReason = LOCTEXT("SCCNotCurrent", "The project file is not at head revision.");
        }
    }

    return bSuccessfullyCheckedOut;
}

void CppToolsUtil::TryMakePluginFileWriteable(const FString& PluginFile)
{
    // See GameProjectUtils.cpp L3629

    // First attempt to check out the file if SCC is enabled
    if ( ISourceControlModule::Get().IsEnabled() )
    {
        FText FailReason;
        CheckoutFile(PluginFile, FailReason);
    }

    // Check if it's writable
    if(FPlatformFileManager::Get().GetPlatformFile().IsReadOnly(*PluginFile))
    {
        FText ShouldMakePluginWriteable = LOCTEXT("ShouldMakePluginWriteable_Message", "'{PluginFilename}' is read-only and cannot be updated. Would you like to make it writeable?");

        FFormatNamedArguments Arguments;
        Arguments.Add( TEXT("PluginFilename"), FText::FromString(PluginFile));

        if(FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(ShouldMakePluginWriteable, Arguments)) == EAppReturnType::Yes)
        {
            FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*PluginFile, false);
        }
    }
}

bool CppToolsUtil::ReadCustomTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason) {
    const FString FullFileName = CppToolsContentDir() / TEXT("Editor") / TEXT("Templates") / TemplateFileName;
    if (FFileHelper::LoadFileToString(OutFileContents, *FullFileName))
    {
        return true;
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(FullFileName));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadTemplateFile", "Failed to read template file \"{FullFileName}\""), Args);
    return false;
}

bool CppToolsUtil::FindFileInProject(const FString& InFilename, const FString& InSearchPath, FString& OutPath)
{
    // See GameProjectUtils L4094
    TArray<FString> Filenames;
    IFileManager::Get().FindFilesRecursive(Filenames, *InSearchPath, *InFilename, true, false, false);

    if(Filenames.Num())
    {
        // Assume it's the first match (we should really only find a single file with a given name within a project anyway)
        OutPath = Filenames[0];
        return true;
    }

    return false;
}

FString CppToolsUtil::StripCStyleComments(const FString& SourceString)
{
    FString Result = SourceString;

    const FString BlockCommentPattern = TEXT("(?:\\/\\*(?:[^\\*]*(?:(?!\\*\\/)\\*)*)*\\*\\/)");
    const FString LineCommentPattern = TEXT("(?:\\/\\/.*)");
    
    const FRegexPattern CommentPattern(BlockCommentPattern + "|" + LineCommentPattern);
    FRegexMatcher CommentMatcher(CommentPattern, Result);

    while (CommentMatcher.FindNext())
    {
        int32 start = CommentMatcher.GetMatchBeginning();
        int32 end = CommentMatcher.GetMatchEnding();

        Result.RemoveAt(start, end - start);

        CommentMatcher = FRegexMatcher(CommentPattern, Result);
    }

    return Result;
}

TArray<FString> CppToolsUtil::ParseStringList(const FString& RawStringList)
{
    TArray<FString> Strings;
    const FRegexPattern StringPattern("\"([^\"]+)\"");
    FRegexMatcher StringMatcher(StringPattern, RawStringList);

    while (StringMatcher.FindNext())
    {
        Strings.Add(StringMatcher.GetCaptureGroup(1));
    }

    return Strings;
}

FString CppToolsUtil::CombineStringList(const TArray<FString>& StringList, bool bUseQuotes = false, bool bIncludeSpace = true)
{
    FString Result;
    for (int32 I = 0; I < StringList.Num(); I++)
    {
        FString Element = StringList[I];
        if (bUseQuotes) Element = "\"" + Element + "\"";

        Result += Element;

        if (I < StringList.Num() - 1)
        {
            Result += ",";
            if (bIncludeSpace) Result += " ";
        }
    }
    return Result;
}

void CppToolsUtil::PushNotification(const FText& Text,
                                    const SNotificationItem::ECompletionState& CompletionState = SNotificationItem::ECompletionState::CS_None)
{
    const FNotificationInfo Notification(Text);
    auto SNotify = FSlateNotificationManager::Get().AddNotification(Notification);
    SNotify->SetCompletionState(CompletionState);
    SNotify->ExpireAndFadeout();
    SNotify.Reset();
}

FString CppToolsUtil::GetCopyrightLine() {
    const FString CopyrightNotice = GetDefault<UGeneralProjectSettings>()->CopyrightNotice;
    if (!CopyrightNotice.IsEmpty())
    {
        return FString(TEXT("// ")) + CopyrightNotice;
    }
    else
    {
        return FString();
    }
}

FString CppToolsUtil::GetModuleAPIMacro(const FString& ModuleName, bool bIsPrivate) {
    return bIsPrivate ? FString() : ModuleName.ToUpper() + "_API ";
}

bool CppToolsUtil::GetModuleBuildFilePath(const FString& ModuleName, TSharedPtr<IPlugin> Target, FString& OutBuildFilePath)
{
    if (!Target.IsValid())
    {
        return FindFileInProject(ModuleName + ".Build.cs", FPaths::GameSourceDir(), OutBuildFilePath);
    }
    return FindFileInProject(ModuleName + ".Build.cs", Target->GetBaseDir(), OutBuildFilePath);
}

TArray<TSharedPtr<IPlugin>> CppToolsUtil::GetProjectPlugins()
{
    TArray<TSharedPtr<IPlugin>> Plugins;
    for (const auto& Plugin : IPluginManager::Get().GetDiscoveredPlugins())
    {
        // Only get plugins that are a part of the game project
        if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project)
        {
            Plugins.Add(MakeShareable(&Plugin.Get()));
        }
    }
    return Plugins;
}

TArray<FModuleContextInfo> CppToolsUtil::GetProjectModules()
{
    return GameProjectUtils::GetCurrentProjectModules();
}

TArray<FModuleContextInfo> CppToolsUtil::GetPluginModules(const TSharedPtr<IPlugin>& Plugin)
{
    // See GameProjectUtils.cpp L2609

    TArray<FModuleContextInfo> AvailableModules;

    if (!Plugin.IsValid()) return AvailableModules;

    // Only get plugins that are a part of the game project
    if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project)
    {
        for (const auto& PluginModule : Plugin->GetDescriptor().Modules)
        {
            FModuleContextInfo ModuleInfo;
            ModuleInfo.ModuleName = PluginModule.Name.ToString();
            ModuleInfo.ModuleType = PluginModule.Type;

            // Try and find the .Build.cs file for this module within the plugin source tree
            FString TmpPath;
            if (!FindFileInProject(ModuleInfo.ModuleName + ".Build.cs", Plugin->GetBaseDir(), TmpPath))
            {
                continue;
            }

            // Chop the .Build.cs file off the end of the path
            ModuleInfo.ModuleSourcePath = FPaths::GetPath(TmpPath);
            ModuleInfo.ModuleSourcePath = FPaths::ConvertRelativePathToFull(ModuleInfo.ModuleSourcePath / ""); // Ensure trailing /

            AvailableModules.Emplace(ModuleInfo);
        }
    }

    return AvailableModules;
}

TArray<FString> CppToolsUtil::GetModuleDependencies(const FString& ModuleName, TSharedPtr<IPlugin> Target = nullptr, bool bIncludePrivate = false)
{
    TArray<FString> Result;
    
    FString BuildFilePath;
    if (!GetModuleBuildFilePath(ModuleName, Target, BuildFilePath)) return Result;

    FString FileContents;
    
    if (FFileHelper::LoadFileToString(FileContents, *BuildFilePath))
    {

        FileContents = StripCStyleComments(FileContents);

        FString DependencySource = "(?:PublicDependencyModuleNames)";
        if (bIncludePrivate) DependencySource += "|(?:PrivateDependencyModuleNames)";
        
        const FString AddRangePattern = DependencySource +
            "\\.AddRange\\(\\s*new\\s+string\\[\\s*\\]\\s*\\{\\s*((?:[^\\}]*(?:\\r\\n|\\r|\\n)?)*)\\s*\\}\\s*\\);";
        const FString AddPattern = DependencySource + "\\.Add\\(([^\\)]+)\\);";
        
        const FRegexPattern DependenciesPattern("(?:" + AddRangePattern + ")|(?:" + AddPattern + ")");
        FRegexMatcher DependenciesMatcher(DependenciesPattern, FileContents);

        while (DependenciesMatcher.FindNext())
        {
            Result.Append(ParseStringList(DependenciesMatcher.GetCaptureGroup(1)));
        }
    }

    return Result;
}

bool CppToolsUtil::GenerateModuleBuildFile(const FString& NewBuildFileName, const FString& ModuleName, const TArray<FString>& PublicDependencyModuleNames,
    const TArray<FString>& PrivateDependencyModuleNames, FText& OutFailReason, bool bUseExplicitOrSharedPCHs)
{
    
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.Build.cs.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PublicDependencyModuleNames), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PRIVATE_DEPENDENCY_MODULE_NAMES%"), *GameProjectUtils::MakeCommaDelimitedList(PrivateDependencyModuleNames), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);

    const FString PCHUsage = bUseExplicitOrSharedPCHs ? TEXT("UseExplicitOrSharedPCHs") : TEXT("UseSharedPCHs");
    FinalOutput = FinalOutput.Replace(TEXT("%PCH_USAGE%"), *PCHUsage, ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewBuildFileName, FinalOutput, OutFailReason);
}

bool CppToolsUtil::GenerateModuleHeaderFile(const FString& NewHeaderFileName, const FString& ModuleName, const TArray<FString>& PublicHeaderIncludes, FText& OutFailReason) {
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.h.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%CLASS_MODULE_API_MACRO%"), *GetModuleAPIMacro(ModuleName, false), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%PUBLIC_HEADER_INCLUDES%"), *GameProjectUtils::MakeIncludeList(PublicHeaderIncludes), ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewHeaderFileName, FinalOutput, OutFailReason);
}

bool CppToolsUtil::GenerateModuleCPPFile(const FString& NewCPPFileName, const FString& ModuleName, const FString& StartupSourceCode, const FString& ShutdownSourceCode, FText& OutFailReason) {
    FString Template;
    if (!ReadCustomTemplateFile(TEXT("Module.cpp.template"), Template, OutFailReason))
    {
        return false;
    }

    FString FinalOutput = Template.Replace(TEXT("%COPYRIGHT_LINE%"), *GetCopyrightLine(), ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_NAME%"), *ModuleName, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_STARTUP_CODE%"), *StartupSourceCode, ESearchCase::CaseSensitive);
    FinalOutput = FinalOutput.Replace(TEXT("%MODULE_SHUTDOWN_CODE%"), *StartupSourceCode, ESearchCase::CaseSensitive);

    return GameProjectUtils::WriteOutputFile(NewCPPFileName, FinalOutput, OutFailReason);
}

bool CppToolsUtil::InsertDependencyIntoModule(const FString& ModuleName, TSharedPtr<IPlugin> Target, const FString& DependencyName, FText& OutFailReason, bool bPrivate = false)
{
    FString FileContents;
    FString BuildFilePath;
    if (!GetModuleBuildFilePath(ModuleName, Target, BuildFilePath))
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("ModuleName"), FText::FromString(ModuleName));
        OutFailReason = FText::Format(LOCTEXT("FailedToReadBuildFile", "Failed to update \"{ModuleName}.Build.cs\""), Args);
        return false;
    }
    
    TArray<FString> Dependencies = GetModuleDependencies(ModuleName, Target, false);
    
    if (FFileHelper::LoadFileToString(FileContents, *BuildFilePath))
    {

        FString DependencySource = "(?:PublicDependencyModuleNames)";
        
        const FString AddRangePattern = DependencySource +
            "\\.AddRange\\(\\s*new\\s+string\\[\\s*\\]\\s*\\{\\s*((?:[^\\}]*(?:\\r\\n|\\r|\\n)?)*)\\s*\\}\\s*\\);";
        const FString AddPattern = DependencySource + "\\.Add\\(([^\\)]+)\\);";
        
        const FRegexPattern DependenciesPattern("(?:" + AddRangePattern + ")|(?:" + AddPattern + ")");
        FRegexMatcher DependenciesMatcher(DependenciesPattern, FileContents);

        int32 lastStart = -1;
        while (DependenciesMatcher.FindNext())
        {
            int32 start = DependenciesMatcher.GetMatchBeginning();
            int32 end = DependenciesMatcher.GetMatchEnding();
            
            TArray<FString> List = ParseStringList(DependenciesMatcher.GetCaptureGroup(1));
            bool bContains = true;
            for (FString Dependency : List)
            {
                bContains &= Dependencies.Contains(Dependency);
            }
            
            if (bContains)
            {
                lastStart = start;
                FileContents.RemoveAt(start, end - start);
            }

            DependenciesMatcher = FRegexMatcher(DependenciesPattern, FileContents);
        }

        Dependencies.Add(DependencyName);

        if (lastStart != -1)
        {
            
            FString InsertionText = TEXT("PublicDependencyModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *CombineStringList(Dependencies, true, true), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(lastStart, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *BuildFilePath);

            return true;
        }
    }

    // Issue modifying or finding file

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(BuildFilePath));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadBuildFile", "Failed to update \"{FullFileName}\""), Args);
    return false;
}

bool CppToolsUtil::InsertDependencyIntoTarget(const FString& ModuleName, const bool& bIsEditor, FText& OutFailReason)
{
    FString FileContents;
    
    FString PrimaryGameTargetName = FApp::GetProjectName();
    if (bIsEditor)
    {
        PrimaryGameTargetName += "Editor";
    }
    
    const FString TargetPath = FPaths::GameSourceDir() / PrimaryGameTargetName + ".Target.cs";
    
    if (FFileHelper::LoadFileToString(FileContents, *TargetPath))
    {

        // Check for existing ExtraModuleNames.AddRange command
        const FRegexPattern ExtraModulesPattern(
            TEXT("ExtraModuleNames\\.AddRange\\([\\s]*new string\\[\\][\\s]*\\{[\\s]*(.*)[\\s]*\\}[\\s]*\\);"));
        FRegexMatcher ExtraModulesMatcher(ExtraModulesPattern, FileContents);

        if (ExtraModulesMatcher.FindNext())
        {
            int32 start = ExtraModulesMatcher.GetMatchBeginning();
            int32 end = ExtraModulesMatcher.GetMatchEnding();

            FileContents.RemoveAt(start, end - start);
            
            FString InsertionText = TEXT("ExtraModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *(ExtraModulesMatcher.GetCaptureGroup(1) + ", \"" + ModuleName + "\""), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(start, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *TargetPath);

            return true;
        }
        
        // Insert new ExtraModuleNames.AddRange command if not found
        /*const FRegexPattern TargetConstructorPattern(TEXT("public[\\s]+") + PrimaryGameTargetName
            + TEXT("[\\s]*\\(.*\\)[\\s]*:[\\s]*base\\(.*\\)[\\s]*\\{.*\\}"));
        FRegexMatcher TargetConstructorMatcher(TargetConstructorPattern, FileContents);

        if (TargetConstructorMatcher.FindNext())
        {
            int32 end = TargetConstructorMatcher.GetMatchEnding();
            
            FString InsertionText = TEXT("\tExtraModuleNames.AddRange( new string[] { MODULES } );");
            InsertionText += LINE_TERMINATOR;
            InsertionText += "\t";
            InsertionText = InsertionText.Replace(TEXT("MODULES"),
                *(PrimaryGameTargetName + ", " + ModuleName), ESearchCase::CaseSensitive);
            
            FileContents.InsertAt(end - 1, InsertionText);

            FFileHelper::SaveStringToFile(FileContents, *TargetPath);

            return true;
        }*/
    }

    // Issue modifying or finding file

    FFormatNamedArguments Args;
    Args.Add(TEXT("FullFileName"), FText::FromString(TargetPath));
    OutFailReason = FText::Format(LOCTEXT("FailedToReadTargetFile", "Failed to update \"{FullFileName}\""), Args);
    return false;
}

GameProjectUtils::EAddCodeToProjectResult CppToolsUtil::GenerateModule(const FString& ModulePath, TSharedPtr<IPlugin> Target,
    const FString& ModuleName, const EHostType::Type& Type, const ELoadingPhase::Type& LoadingPhase, bool bUsePCH,
    TArray<FString>& CreatedFiles, FText& OutFailReason)
{

    //TODO: Add data validation

    TArray<FModuleDescriptor> GeneratedModules;

    FScopedSlowTask SlowTask(11, LOCTEXT("AddingModuleToProject", "Adding module to project..."));
    SlowTask.MakeDialog();

    SlowTask.EnterProgressFrame();

    // Module.Build.cs
    {
        const FString BuildFilename = ModulePath / ModuleName + TEXT(".Build.cs");
        TArray<FString> PublicDependencyModuleNames;
        PublicDependencyModuleNames.Add(TEXT("Core"));
        PublicDependencyModuleNames.Add(TEXT("CoreUObject"));
        PublicDependencyModuleNames.Add(TEXT("Engine"));
        PublicDependencyModuleNames.Add(TEXT("InputCore"));
        TArray<FString> PrivateDependencyModuleNames;
        if (CppToolsUtil::GenerateModuleBuildFile(BuildFilename, ModuleName, PublicDependencyModuleNames, PrivateDependencyModuleNames, OutFailReason, bUsePCH)) {
            GeneratedModules.Add(FModuleDescriptor(*ModuleName, Type, LoadingPhase));
            CreatedFiles.Add(BuildFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    // Module Header
    {
        const FString HeaderFilename = ModulePath / "Public" / ModuleName + TEXT(".h");
        TArray<FString> PublicHeaderIncludes;
        if (CppToolsUtil::GenerateModuleHeaderFile(HeaderFilename, ModuleName, PublicHeaderIncludes, OutFailReason)) {
            CreatedFiles.Add(HeaderFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    // Module Source
    {
        const FString SourceFilename = ModulePath / "Private" / ModuleName + TEXT(".cpp");
        FString StartupSource;
        FString ShutdownSource;
        if (CppToolsUtil::GenerateModuleCPPFile(SourceFilename, ModuleName, StartupSource, ShutdownSource, OutFailReason)) {
            CreatedFiles.Add(SourceFilename);
        }
        else {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    // Add to appropriate module .Build.cs
    {
        FString OwningModule;
        if (!Target.IsValid())
        {
            OwningModule = FApp::GetProjectName();
            if (Type == EHostType::Editor) OwningModule += "Editor";
        }
        else
        {
            FString ExpectedOwner = Target->GetName();
            if (Type == EHostType::Editor) ExpectedOwner += "Editor";
            bool bFoundOwner = false;

            // Search for a compatible module
            // TODO: Replace this with specifying users during module creation
            
            TArray<FModuleContextInfo> Modules = GetPluginModules(Target);
            if (Modules.Num() > 0)
            {
                for (FModuleContextInfo Module : Modules)
                {
                    if (Module.ModuleName == ExpectedOwner)
                    {
                        OwningModule = ExpectedOwner;
                        bFoundOwner = true;
                        break;
                    }
                }
                if (!bFoundOwner)
                {
                    for (FModuleContextInfo Module : Modules)
                    {
                        if (Module.ModuleType == Type)
                        {
                            OwningModule = Module.ModuleName;
                            break;
                        }
                    }
                }
            }
        }
        // Only add to the build if the module expected to use it exists
        if (FModuleManager::Get().ModuleExists(*OwningModule) &&
            !InsertDependencyIntoModule(OwningModule, Target, ModuleName, OutFailReason, false))
        {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }
    
    SlowTask.EnterProgressFrame();

    // Add to project target
    if (!Target.IsValid()) {
        if (!InsertDependencyIntoTarget(ModuleName, Type == EHostType::Editor, OutFailReason))
        {
            GameProjectUtils::DeleteCreatedFiles(ModulePath, CreatedFiles);
            return GameProjectUtils::EAddCodeToProjectResult::FailedToAddCode;
        }
    }

    SlowTask.EnterProgressFrame();

    if (!Target.IsValid())
    {
        // Update .uproject file
        auto Modifier = FProjectDescriptorModifier::CreateLambda(
            [&GeneratedModules](FProjectDescriptor& Descriptor)
            {
                // See GameProjectUtils.cpp L3920
                bool bNeedsUpdate = false;

                bNeedsUpdate |= AppendProjectModules(Descriptor, &GeneratedModules);
                //bNeedsUpdate |= UpdateRequiredAdditionalDependencies(Descriptor, RequiredDependencies, ModuleInfo.ModuleName);

                return bNeedsUpdate;
            });

        UpdateGameProject(&Modifier);
    }
    else
    {
        // Update .uplugin file
        auto Modifier = FPluginDescriptorModifier::CreateLambda(
            [&GeneratedModules, &Target](FPluginDescriptor& Descriptor)
            {
                bool bNeedsUpdate = false;
                bNeedsUpdate |= AppendPluginModules(Descriptor, Target, &GeneratedModules);
                return bNeedsUpdate;
            });

        UpdatePlugin(Target, &Modifier);
    }

    SlowTask.EnterProgressFrame();

    // Rebuild project

    FString ProjectFileName = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::GetProjectFilePath());
    //FString Arguments = FString::Printf(TEXT("%s %s %s -Plugin=\"%s\" -Project=\"%s\" -Progress -NoHotReloadFromIDE"), FPlatformMisc::GetUBTTargetName(), FModuleManager::Get().GetUBTConfiguration(), FPlatformMisc::GetUBTPlatform(), *UPluginFilePath, *ProjectFileName);
    FString Arguments = FString::Printf(TEXT("%s %s %s -Project=\"%s\" -Progress -NoHotReloadFromIDE"), FPlatformMisc::GetUBTTargetName(), FModuleManager::Get().GetUBTConfiguration(), FPlatformMisc::GetUBTPlatform(), *ProjectFileName);

    if (!FDesktopPlatformModule::Get()->RunUnrealBuildTool(LOCTEXT("Compiling", "Compiling..."), FPaths::RootDir(), Arguments, GWarn)) {
        // Failed to compile
    }

    FModuleManager::Get().ResetModulePathsCache();

    SlowTask.EnterProgressFrame();

    TArray<FString> CreatedFilesForExternalAppRead;
    CreatedFilesForExternalAppRead.Reserve(CreatedFiles.Num());
    for (const FString& CreatedFile : CreatedFiles)
    {
        CreatedFilesForExternalAppRead.Add(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*CreatedFile));
    }

    SlowTask.EnterProgressFrame();

    // Mark the files for add in SCC
    ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
    if (ISourceControlModule::Get().IsEnabled() && SourceControlProvider.IsAvailable())
    {
        SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), CreatedFilesForExternalAppRead);
    }

    SlowTask.EnterProgressFrame();

    // Attempt to add files to solution
    if (!FSourceCodeNavigation::AddSourceFiles(CreatedFilesForExternalAppRead))
	{
		// Generate project files if we happen to be using a project file.
		if ( !FDesktopPlatformModule::Get()->GenerateProjectFiles(FPaths::RootDir(), FPaths::GetProjectFilePath(), GWarn) )
		{
			OutFailReason = LOCTEXT("FailedToGenerateProjectFiles", "Failed to generate project files.");
			return GameProjectUtils::EAddCodeToProjectResult::FailedToHotReload;
		}
	}

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("CompilingCPlusPlusCode", "Compiling new C++ code.  Please wait..."));

    // Hot reload files
    // See: GameProjectUtils.cpp L4043
    
    IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
    if (!HotReloadSupport.RecompileModule(*ModuleName, *GWarn, ERecompileModuleFlags::ReloadAfterRecompile))
    {
        OutFailReason = LOCTEXT("FailedToCompileNewModule", "Failed to compile newly created module.");
        return GameProjectUtils::EAddCodeToProjectResult::FailedToHotReload;
    }

    FSourceCodeNavigation::AccessOnNewModuleAdded().Broadcast(*ModuleName);

    return GameProjectUtils::EAddCodeToProjectResult::Succeeded;
}

bool CppToolsUtil::UpdateGameProjectFile(const FString& ProjectFile, const FString& EngineIdentifier, const FProjectDescriptorModifier* Modifier, FText& OutFailReason) {

    // See GameProjectUtils.cpp L3661

    GameProjectUtils::TryMakeProjectFileWriteable(ProjectFile);

    FProjectDescriptor Descriptor;
    if (Descriptor.Load(ProjectFile, OutFailReason)) {
        if (Modifier && Modifier->IsBound() && !Modifier->Execute(Descriptor)) {
            return true;
        }
        return Descriptor.Save(ProjectFile, OutFailReason) && FDesktopPlatformModule::Get()->SetEngineIdentifierForProject(ProjectFile, EngineIdentifier);
    }
    return false;
}

bool CppToolsUtil::UpdateGameProject(const FProjectDescriptorModifier* Modifier) {

    // See GameProjectUtils.cpp L3568

    const FString& ProjectFilename = FPaths::GetProjectFilePath();
    const FString& ShortFilename = FPaths::GetCleanFilename(ProjectFilename);

    FText FailReason;
    FText UpdateMessage;
    SNotificationItem::ECompletionState NewCompletionState;

    if (UpdateGameProjectFile(ProjectFilename, FDesktopPlatformModule::Get()->GetCurrentEngineIdentifier(), Modifier, FailReason)) {
        // Project Updated Successfully
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        UpdateMessage = FText::Format(LOCTEXT("ProjectFileUpdateComplete", "{ShortFilename} was successfully updated."), Args);
        NewCompletionState = SNotificationItem::CS_Success;
        PushNotification(UpdateMessage, NewCompletionState);
        return true;
    }
    else {
        // The user chose to update, but the update failed. Notify the user.
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        Args.Add(TEXT("FailReason"), FailReason);
        UpdateMessage = FText::Format(LOCTEXT("ProjectFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args);
        NewCompletionState = SNotificationItem::CS_Fail;
        PushNotification(UpdateMessage, NewCompletionState);
        return false;
    }
}

bool CppToolsUtil::AppendProjectModules(FProjectDescriptor& Descriptor, const TArray<FModuleDescriptor>* Modules) {
    // See GameProjectUtils.cpp L1070
    if (Modules == nullptr) return false;

    for (int32 Idx = 0; Idx < Modules->Num(); Idx++)
    {
        auto Element = (*Modules)[Idx];
        bool bContains = false;
        bool bUpdatedPrimary = false;
        for (int32 I = 0; I < Descriptor.Modules.Num(); I++) {
            if (!bContains && Descriptor.Modules[I].Name == Element.Name) {
                bContains = true;
                if (bUpdatedPrimary) break;
            }
            if (!bUpdatedPrimary && Descriptor.Modules[I].Name.ToString() == FString(FApp::GetProjectName())
                + (Descriptor.Modules[I].Type == EHostType::Editor ? "Editor" : ""))
            {
                auto& AdditionalDependencies = Descriptor.Modules[I].AdditionalDependencies;
                if (!AdditionalDependencies.Contains(Element.Name.ToString()))
                {
                    AdditionalDependencies.Add(Element.Name.ToString());
                }
                bUpdatedPrimary = true;
                if (bContains) break;
            }
        }
        if (!bContains) Descriptor.Modules.Add(Element);
    }

    GameProjectUtils::ResetCurrentProjectModulesCache();

    return true;
}

bool CppToolsUtil::UpdatePluginFile(const FString& PluginFile, const FPluginDescriptorModifier* Modifier, FText& OutFailReason)
{
    TryMakePluginFileWriteable(PluginFile);

    FPluginDescriptor Descriptor;
    if (Descriptor.Load(PluginFile, OutFailReason)) {
        if (Modifier && Modifier->IsBound() && !Modifier->Execute(Descriptor)) {
            return true;
        }
        return Descriptor.Save(PluginFile, OutFailReason);
    }
    return false;
}

bool CppToolsUtil::UpdatePlugin(const TSharedPtr<IPlugin>& Plugin, const FPluginDescriptorModifier* Modifier)
{
    if (!Plugin.IsValid()) return false;
    
    const FString& PluginFilename = Plugin->GetDescriptorFileName();
    const FString& ShortFilename = FPaths::GetCleanFilename(PluginFilename);

    FText FailReason;
    FText UpdateMessage;
    SNotificationItem::ECompletionState NewCompletionState;

    if (UpdatePluginFile(PluginFilename, Modifier, FailReason)) {
        // Plugin Updated Successfully
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        UpdateMessage = FText::Format(LOCTEXT("PluginFileUpdateComplete", "{ShortFilename} was successfully updated."), Args);
        NewCompletionState = SNotificationItem::CS_Success;
        PushNotification(UpdateMessage, NewCompletionState);
        return true;
    }
    else {
        // The user chose to update, but the update failed. Notify the user.
        FFormatNamedArguments Args;
        Args.Add(TEXT("ShortFilename"), FText::FromString(ShortFilename));
        Args.Add(TEXT("FailReason"), FailReason);
        UpdateMessage = FText::Format(LOCTEXT("PluginFileUpdateFailed", "{ShortFilename} failed to update. {FailReason}"), Args);
        NewCompletionState = SNotificationItem::CS_Fail;
        PushNotification(UpdateMessage, NewCompletionState);
        return false;
    }
}

bool CppToolsUtil::AppendPluginModules(FPluginDescriptor& Descriptor, TSharedPtr<IPlugin> Plugin, const TArray<FModuleDescriptor>* Modules)
{
    if (Modules == nullptr) return false;

    for (int32 Idx = 0; Idx < Modules->Num(); Idx++)
    {
        auto Element = (*Modules)[Idx];
        bool bContains = false;
        bool bUpdatedPrimary = false;
        for (int32 I = 0; I < Descriptor.Modules.Num(); I++) {
            if (!bContains && Descriptor.Modules[I].Name == Element.Name) {
                bContains = true;
                if (bUpdatedPrimary) break;
            }
            if (!bUpdatedPrimary && Descriptor.Modules[I].Name.ToString() == Plugin->GetName()
                + (Descriptor.Modules[I].Type == EHostType::Editor ? "Editor" : ""))
            {
                auto& AdditionalDependencies = Descriptor.Modules[I].AdditionalDependencies;
                if (!AdditionalDependencies.Contains(Element.Name.ToString()))
                {
                    AdditionalDependencies.Add(Element.Name.ToString());
                }
                bUpdatedPrimary = true;
                if (bContains) break;
            }
        }
        if (!bContains) Descriptor.Modules.Add(Element);
    }

    GameProjectUtils::ResetCurrentProjectModulesCache();

    return true;
}

#undef LOCTEXT_NAMESPACE
