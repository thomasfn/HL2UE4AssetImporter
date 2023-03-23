#include "HL2Editor.h"

#include "IHL2Runtime.h"
#include "Engine/Texture.h"
#include "BSPImporter.h"
#include "Framework/SlateDelegates.h"
#include "UtilMenuCommands.h"
#include "UtilMenuStyle.h"
#include "DesktopPlatformModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetToolsModule.h"
#include "PackageHelperFunctions.h"
#include "HAL/PlatformFilemanager.h"
#include "VMTMaterial.h"
#include "MaterialUtils.h"
#include "SkyboxConverter.h"
#include "TerrainTracer.h"
#include "Engine/Selection.h"
#include "SoundScriptFactory.h"
#include "SoundScapeFactory.h"
#include "BlueprintEditorModule.h"
#include "HL2VariableDetailsCustomization.h"

DEFINE_LOG_CATEGORY(LogHL2Editor);

#define LOCTEXT_NAMESPACE ""

void HL2EditorImpl::StartupModule()
{
	vtfLibDllHandle = FPlatformProcess::GetDllHandle(*GetVTFLibDllPath());
	check(vtfLibDllHandle);
	mpg123DllHandle = FPlatformProcess::GetDllHandle(*GetMPG123LibDllPath());
	check(mpg123DllHandle);

	FUtilMenuStyle::Initialize();

	FUtilMenuCommands::Register();

	utilMenuCommandList = MakeShareable(new FUICommandList);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportTextures,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportTexturesClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportMaterials,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportMaterialsClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportModels,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportModelsClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportSounds,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportSoundsClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().ImportScripts,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::ImportScriptsClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().ConvertSkyboxes,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::ConvertSkyboxes),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().ImportBSP,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::ImportBSPClicked),
		FCanExecuteAction()
	);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().TraceTerrain,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::TraceTerrainClicked),
		FCanExecuteAction()
	);

	myExtender = MakeShareable(new FExtender);
	myExtender->AddToolBarExtension("Content", EExtensionHook::After, NULL, FToolBarExtensionDelegate::CreateRaw(this, &HL2EditorImpl::AddToolbarExtension));

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->AddExtender(myExtender);

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.RegisterVariableCustomization(
		FProperty::StaticClass(),
		FOnGetVariableCustomizationInstance::CreateStatic(&FHL2VariableDetailsCustomization::MakeInstance)
	);
}

void HL2EditorImpl::ShutdownModule()
{
	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(myExtender);

	FUtilMenuCommands::Unregister();

	FUtilMenuStyle::Shutdown();

	FPlatformProcess::FreeDllHandle(vtfLibDllHandle);
}

const FHL2EditorConfig& HL2EditorImpl::GetConfig() const
{
	return editorConfig;
}

void HL2EditorImpl::AddToolbarExtension(FToolBarBuilder& builder)
{
	FUIAction UtilMenuAction;

	builder.AddComboButton(
		UtilMenuAction,
		FOnGetContent::CreateStatic(&HL2EditorImpl::GenerateUtilityMenu, utilMenuCommandList.ToSharedRef()),
		LOCTEXT("HL2UtilMenu_Label", "HL2"),
		LOCTEXT("HL2UtilMenu_Tooltip", "HL2 Importer Utilities"),
		FSlateIcon(FUtilMenuStyle::GetStyleSetName(), "HL2AssetImporter.UtilityMenu")
	);
}

TSharedRef<SWidget> HL2EditorImpl::GenerateUtilityMenu(TSharedRef<FUICommandList> commandList)
{
	FMenuBuilder menuBuilder(true, commandList);

	menuBuilder.BeginSection("Asset Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportTextures);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportMaterials);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportModels);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportSounds);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().ImportScripts);
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().ConvertSkyboxes);
	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Map Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().ImportBSP);
	}
	menuBuilder.EndSection();

	menuBuilder.BeginSection("Tools");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().TraceTerrain);
	}
	menuBuilder.EndSection();

	return menuBuilder.MakeWidget();
}

void HL2EditorImpl::BulkImportTexturesClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose VTF Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".vtf"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("TexturesImporting", "Importing vtfs..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2TextureBasePath() / dir);
			SaveImportedAssets(importedAssets);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::BulkImportMaterialsClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose VMT Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".vmt"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("MaterialsImporting", "Importing vmts..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2MaterialBasePath() / dir);
			SaveImportedAssets(importedAssets);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::BulkImportModelsClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose MDL Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".mdl"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);
	filesToImport = filesToImport.FilterByPredicate([](const FString& file)
		{
			if (file.Contains(TEXT("_animations"), ESearchCase::IgnoreCase)) { return false; }
			if (file.Contains(TEXT("_anims"), ESearchCase::IgnoreCase)) { return false; }
			if (file.Contains(TEXT("_gestures"), ESearchCase::IgnoreCase)) { return false; }
			if (file.Contains(TEXT("_postures"), ESearchCase::IgnoreCase)) { return false; }
			return true;
		});

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("ModelsImporting", "Importing mdls..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2ModelBasePath() / dir);
			SaveImportedAssets(importedAssets);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::BulkImportSoundsClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose Sound Location"), TEXT(""), rootPath)) { return; }
	rootPath += "/";

	// Scan folder
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	TArray<FString> filesToImport;
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".wav"));
	platformFile.FindFilesRecursively(filesToImport, *rootPath, TEXT(".mp3"));
	UE_LOG(LogHL2Editor, Log, TEXT("Importing %d files from '%s'..."), filesToImport.Num(), *rootPath);

	// Import all
	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	TMap<FString, TArray<FString>> groupedFilesToImport;
	GroupFileListByDirectory(filesToImport, groupedFilesToImport);
	FScopedSlowTask loopProgress(groupedFilesToImport.Num(), LOCTEXT("SoundsImporting", "Importing wavs/mp3s..."));
	loopProgress.MakeDialog();
	for (const auto& pair : groupedFilesToImport)
	{
		FString dir = pair.Key;
		if (FPaths::MakePathRelativeTo(dir, *rootPath))
		{
			loopProgress.EnterProgressFrame();
			TArray<UObject*> importedAssets = assetTools.ImportAssets(pair.Value, IHL2Runtime::Get().GetHL2SoundBasePath() / dir);
			SaveImportedAssets(importedAssets);
			UE_LOG(LogHL2Editor, Log, TEXT("Imported %d assets to '%s'"), importedAssets.Num(), *dir);
		}
	}
}

void HL2EditorImpl::ImportScriptsClicked()
{
	// Ask user to select folder to import from
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	FString rootPath;
	if (!desktopPlatform->OpenDirectoryDialog(nullptr, TEXT("Choose Scripts Location"), TEXT(""), rootPath)) { return; }


	IAssetTools& assetTools = FAssetToolsModule::GetModule().Get();
	FScopedSlowTask loopProgress(2, LOCTEXT("ScriptsImporting", "Importing scripts..."));
	loopProgress.MakeDialog();

	// Import sound scripts
	{
		USoundScriptFactory* factory = NewObject<USoundScriptFactory>();
		loopProgress.EnterProgressFrame(1.0f, LOCTEXT("SoundScripts", "Sound scripts"));
		UAutomatedAssetImportData* importData = NewObject<UAutomatedAssetImportData>();
		importData->Filenames.Add(rootPath / "game_sounds_manifest.txt");
		importData->DestinationPath = IHL2Runtime::Get().GetHL2ScriptBasePath();
		importData->FactoryName = TEXT("SoundScriptFactory");
		importData->bReplaceExisting = true;
		importData->Factory = factory;
		TArray<UObject*> importedAssets = assetTools.ImportAssetsAutomated(importData);
		SaveImportedAssets(importedAssets);
		UE_LOG(LogHL2Editor, Log, TEXT("Imported %d sound script assets"), importedAssets.Num());	
	}

	// Import soundscapes
	{
		USoundScapeFactory* factory = NewObject<USoundScapeFactory>();
		loopProgress.EnterProgressFrame(1.0f, LOCTEXT("SoundScapes", "Sound scapes"));
		UAutomatedAssetImportData* importData = NewObject<UAutomatedAssetImportData>();
		importData->Filenames.Add(rootPath / "soundscapes_manifest.txt");
		importData->DestinationPath = IHL2Runtime::Get().GetHL2ScriptBasePath();
		importData->FactoryName = TEXT("SoundScapeFactory");
		importData->bReplaceExisting = true;
		importData->Factory = factory;
		TArray<UObject*> importedAssets = assetTools.ImportAssetsAutomated(importData);
		SaveImportedAssets(importedAssets);
		UE_LOG(LogHL2Editor, Log, TEXT("Imported %d sound scape assets"), importedAssets.Num());
	}
}

void HL2EditorImpl::ConvertSkyboxes()
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();

	TArray<FAssetData> assetDatas;
	FString texturePath = IHL2Runtime::Get().GetHL2TextureBasePath() / TEXT("skybox");
	assetRegistry.GetAssetsByPath(FName(*texturePath), assetDatas);

	TArray<FAssetData> selectedAssetDatas;
	FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& contentBrowserSingleton = contentBrowserModule.Get();
	contentBrowserSingleton.GetSelectedAssets(selectedAssetDatas);

	TArray<FString> skyboxNames;

	if (selectedAssetDatas.Num() > 0)
	{
		for (const FAssetData& assetData : selectedAssetDatas)
		{
			FString assetName = assetData.AssetName.ToString();
			FString skyboxName;
			if (assetData.GetClass() == UTexture2D::StaticClass() && FSkyboxConverter::IsSkyboxTexture(assetName, skyboxName))
			{
				skyboxNames.AddUnique(skyboxName);
			}
		}
	}
	else
	{
		for (const FAssetData& assetData : assetDatas)
		{
			FString assetName = assetData.AssetName.ToString();
			FString skyboxName;
			if (assetData.GetClass() == UTexture2D::StaticClass() && FSkyboxConverter::IsSkyboxTexture(assetName, skyboxName))
			{
				skyboxNames.AddUnique(skyboxName);
			}
		}
	}
	
	FScopedSlowTask loopProgress(skyboxNames.Num(), LOCTEXT("SkyboxesConverting", "Converting skyboxes to cubemaps..."));
	loopProgress.MakeDialog();
	for (const FString& skyboxName : skyboxNames)
	{
		loopProgress.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("SkyboxesConverting_Individual", "{0}"), FText::FromString(skyboxName)));
		FString packageName = IHL2Runtime::Get().GetHL2TextureBasePath() / TEXT("skybox") / skyboxName;
		FAssetData existingAsset = assetRegistry.GetAssetByObjectPath(FSoftObjectPath(packageName + TEXT(".") + skyboxName));
		if (existingAsset.IsValid())
		{
			UTextureCube* existingTexture = CastChecked<UTextureCube>(existingAsset.GetAsset());
			FSkyboxConverter::ConvertSkybox(existingTexture, skyboxName);
			existingTexture->PostEditChange();
			existingTexture->MarkPackageDirty();
		}
		else
		{
			UPackage* package = CreatePackage(*packageName);
			UTextureCube* cubeMap = NewObject<UTextureCube>(package, FName(*skyboxName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			FSkyboxConverter::ConvertSkybox(cubeMap, skyboxName);
			cubeMap->PostEditChange();
			FAssetRegistryModule::AssetCreated(cubeMap);
			cubeMap->MarkPackageDirty();
		}
	}
}

void HL2EditorImpl::ImportBSPClicked()
{
	// Ask user to select BSP to import
	IDesktopPlatform* desktopPlatform = FDesktopPlatformModule::Get();
	TArray<FString> selectedFilenames;
	if (!desktopPlatform->OpenFileDialog(nullptr, "Import BSP", TEXT(""), TEXT(""), TEXT("bsp;Valve Map File"), EFileDialogFlags::None, selectedFilenames)) { return; }
	if (selectedFilenames.Num() != 1) { return; }
	const FString& fileName = selectedFilenames[0];

	// Run import
	FBSPImporter importer(fileName);
	if (importer.Load())
	{
		importer.ImportToCurrentLevel();
	}
}

void HL2EditorImpl::TraceTerrainClicked()
{
	USelection* selection = GEditor->GetSelectedActors();
	TArray<ALandscape*> selectedLandscapes;
	selection->GetSelectedObjects(selectedLandscapes);
	for (ALandscape* landscape : selectedLandscapes)
	{
		FTerrainTracer tracer(landscape);
		tracer.Trace();
	}
}

void HL2EditorImpl::SaveImportedAssets(TArrayView<UObject*> importedObjects)
{
	for (UObject* obj : importedObjects)
	{
		UPackage* package = CastChecked<UPackage>(obj->GetOuter());
		FString filename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(package->GetName(), filename, FPackageName::GetAssetPackageExtension())) { continue; }
		SavePackageHelper(package, filename);
	}
}

void HL2EditorImpl::GroupFileListByDirectory(const TArray<FString>& files, TMap<FString, TArray<FString>>& outMap)
{
	for (const FString& file : files)
	{
		FString path = FPaths::GetPath(file);
		if (!outMap.Contains(path))
		{
			outMap.Add(path, TArray<FString>());
		}
		outMap[path].Add(file);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(HL2EditorImpl, HL2Editor)