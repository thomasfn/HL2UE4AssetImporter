#include "HL2EditorPrivatePCH.h"

#include "IHL2Editor.h"
#include "HL2Editor.h"

#include "Misc/Paths.h"
#include "Engine/Texture.h"
#include "VMTMaterial.h"

#ifdef WITH_EDITOR
#include "Framework/SlateDelegates.h"
#include "UtilMenuCommands.h"
#include "UtilMenuStyle.h"
#endif

DEFINE_LOG_CATEGORY(LogHL2Editor);

void HL2EditorImpl::StartupModule()
{
#ifdef WITH_EDITOR

	isLoading = true;

	FUtilMenuStyle::Initialize();

	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	handleFilesLoaded = assetRegistry.OnFilesLoaded().AddRaw(this, &HL2EditorImpl::HandleFilesLoaded);
	handleAssetAdded = assetRegistry.OnAssetAdded().AddRaw(this, &HL2EditorImpl::HandleAssetAdded);

	FUtilMenuCommands::Register();

	utilMenuCommandList = MakeShareable(new FUICommandList);
	utilMenuCommandList->MapAction(
		FUtilMenuCommands::Get().BulkImportTextures,
		FExecuteAction::CreateRaw(this, &HL2EditorImpl::BulkImportTexturesClicked),
		FCanExecuteAction()
	);

	myExtender = MakeShareable(new FExtender);
	myExtender->AddToolBarExtension("Content", EExtensionHook::After, NULL, FToolBarExtensionDelegate::CreateRaw(this, &HL2EditorImpl::AddToolbarExtension));

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->AddExtender(myExtender);

#endif
}

void HL2EditorImpl::ShutdownModule()
{
#ifdef WITH_EDITOR

	handleAssetAdded.Reset();
	handleFilesLoaded.Reset();

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	levelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(myExtender);

	FUtilMenuCommands::Unregister();

	FUtilMenuStyle::Shutdown();

#endif
}

#ifdef WITH_EDITOR

void HL2EditorImpl::HandleFilesLoaded()
{
	isLoading = false;
	handleFilesLoaded.Reset();
}

void HL2EditorImpl::HandleAssetAdded(const FAssetData& assetData)
{
	if (isLoading) { return; }
	if (Cast<UTexture>(assetData.GetAsset()))
	{
		TArray<UVMTMaterial*> materials;
		FindAllMaterialsThatReferenceTexture(assetData.ObjectPath, materials);
		for (UVMTMaterial* material : materials)
		{
			material->TryResolveTextures();
			material->PostEditChange();
			material->MarkPackageDirty();
			UE_LOG(LogHL2Editor, Log, TEXT("Trying to resolve textures on '%s' as '%s' now exists"), *material->GetName(), *assetData.AssetName.ToString());
		}
	}
}

#define LOCTEXT_NAMESPACE ""

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

#undef LOCTEXT_NAMESPACE

TSharedRef<SWidget> HL2EditorImpl::GenerateUtilityMenu(TSharedRef<FUICommandList> commandList)
{
	FMenuBuilder menuBuilder(true, commandList);

	menuBuilder.BeginSection("Asset Import");
	{
		menuBuilder.AddMenuEntry(FUtilMenuCommands::Get().BulkImportTextures);
	}
	menuBuilder.EndSection();

	return menuBuilder.MakeWidget();
}

void HL2EditorImpl::BulkImportTexturesClicked()
{
	UE_LOG(LogHL2Editor, Log, TEXT("BulkImportTexturesClicked"));
}

#endif

FName HL2EditorImpl::HL2TexturePathToAssetPath(const FString& hl2TexturePath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a.brickfloor001a"
	FString tmp = hl2TexturePath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2TextureBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2EditorImpl::HL2MaterialPathToAssetPath(const FString& hl2MaterialPath) const
{
	// e.g. "Brick/brickfloor001a" -> "/Content/hl2/materials/Brick/brickfloor001a_vmt.brickfloor001a_vmt"
	FString tmp = hl2MaterialPath;
	tmp.ReplaceCharInline('\\', '/');
	return FName(*(hl2TextureBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

FName HL2EditorImpl::HL2ShaderPathToAssetPath(const FString& hl2ShaderPath, bool translucent) const
{
	// e.g. "VertexLitGeneric" -> "/HL2Editor/Shaders/VertexLitGeneric.VertexLitGeneric"
	FString tmp = hl2ShaderPath;
	tmp.ReplaceCharInline('\\', '/');
	if (translucent) { tmp = tmp.Append("_translucent"); }
	return FName(*(hl2ShaderBasePath + tmp + '.' + FPaths::GetCleanFilename(tmp)));
}

UTexture* HL2EditorImpl::TryResolveHL2Texture(const FString& hl2TexturePath) const
{
	FName assetPath = HL2TexturePathToAssetPath(hl2TexturePath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UTexture>(assetData.GetAsset());
}

UVMTMaterial* HL2EditorImpl::TryResolveHL2Material(const FString& hl2MaterialPath) const
{
	FName assetPath = HL2MaterialPathToAssetPath(hl2MaterialPath);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UVMTMaterial>(assetData.GetAsset());
}

UMaterial* HL2EditorImpl::TryResolveHL2Shader(const FString& hl2ShaderPath, bool translucent) const
{
	FName assetPath = HL2ShaderPathToAssetPath(hl2ShaderPath, translucent);
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	FAssetData assetData = assetRegistry.GetAssetByObjectPath(assetPath);
	if (!assetData.IsValid()) { return nullptr; }
	return CastChecked<UMaterial>(assetData.GetAsset());
}

void HL2EditorImpl::FindAllMaterialsThatReferenceTexture(const FString& hl2TexturePath, TArray<UVMTMaterial*>& out) const
{
	FindAllMaterialsThatReferenceTexture(HL2TexturePathToAssetPath(hl2TexturePath), out);
}

void HL2EditorImpl::FindAllMaterialsThatReferenceTexture(FName assetPath, TArray<UVMTMaterial*>& out) const
{
	FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& assetRegistry = assetRegistryModule.Get();
	TArray<FAssetData> assets;
	assetRegistry.GetAssetsByClass(UVMTMaterial::StaticClass()->GetFName(), assets);
	for (const FAssetData& assetData : assets)
	{
		UVMTMaterial* material = Cast<UVMTMaterial>(assetData.GetAsset());
		if (material && material->DoesReferenceTexture(assetPath))
		{
			out.Add(material);
		}
	}
}

IMPLEMENT_MODULE(HL2EditorImpl, HL2Editor)