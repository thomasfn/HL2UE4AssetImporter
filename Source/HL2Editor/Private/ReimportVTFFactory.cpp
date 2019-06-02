#include "HL2EditorPrivatePCH.h"

#include "IHL2Editor.h"
#include "ReimportVTFFactory.h"

#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"
#include "Runtime/Engine/Classes/Engine/VolumeTexture.h"
#include "Runtime/Engine/Classes/Engine/TextureCube.h"
#include "Runtime/JsonUtilities/Public/JsonObjectConverter.h"

UReimportVTFFactory::UReimportVTFFactory()
{
	SupportedClass = UTexture::StaticClass();

	bCreateNew = false;

	++ImportPriority;
}

UReimportVTFFactory::~UReimportVTFFactory()
{
	
}

UTexture2D* UReimportVTFFactory::CreateTexture2D(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UTexture2D* pTex2D = Cast<UTexture2D>(pOriginalTex);

	if (pTex2D)
	{
		// Release the existing resource so the new texture can get a fresh one. Otherwise if the next call to Init changes the format
		// of the texture and UpdateResource is called the editor will crash in RenderThread
		pTex2D->ReleaseResource();
		return pTex2D;
	}
	else
	{
		return Super::CreateTexture2D(InParent, Name, Flags);
	}
}

UTextureCube* UReimportVTFFactory::CreateTextureCube(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UTextureCube* pTexCube = Cast<UTextureCube>(pOriginalTex);
	if (pTexCube)
	{
		// Release the existing resource so the new texture can get a fresh one. Otherwise if the next call to Init changes the format
		// of the texture and UpdateResource is called the editor will crash in RenderThread
		pTexCube->ReleaseResource();
		return pTexCube;
	}
	else
	{
		return Super::CreateTextureCube(InParent, Name, Flags);
	}
}

bool UReimportVTFFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UTexture* pTex = Cast<UTexture>(Obj);
	if (pTex && pTex->AssetImportData)
	{
		TArray<FString> fileNames = pTex->AssetImportData->ExtractFilenames();
		for (const FString& fileName : fileNames)
		{
			if (!FactoryCanImport(fileName))
			{
				return false;
			}
		}
		OutFilenames.Append(fileNames);
		return true;
	}
	return false;
}

void UReimportVTFFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UTexture* pTex = Cast<UTexture>(Obj);
	if (pTex && ensure(NewReimportPaths.Num() == 1))
	{
		pTex->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

/**
* Reimports specified texture from its source material, if the meta-data exists
*/
EReimportResult::Type UReimportVTFFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UTexture::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UTexture* pTex = Cast<UTexture>(Obj);

	TGuardValue<UTexture*> OriginalTexGuardValue(pOriginalTex, pTex);

	const FString ResolvedSourceFilePath = pTex->AssetImportData->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		// Since this is a new system most textures don't have paths, so logging has been commented out
		//UE_LOG(LogHL2Editor, Warning, TEXT("-- cannot reimport: texture resource does not have path stored."));
		return EReimportResult::Failed;
	}

	UTexture2D* pTex2D = Cast<UTexture2D>(Obj);
	// Check if this texture has been modified by the paint tool.
	// If so, prompt the user to see if they'll continue with reimporting, returning if they decline.
	if (pTex2D && pTex2D->bHasBeenPaintedInEditor && EAppReturnType::Yes != FMessageDialog::Open(EAppMsgType::YesNo,
		FText::Format(NSLOCTEXT("UnrealEd", "Import_TextureHasBeenPaintedInEditor", "The texture '{0}' has been painted on by the Mesh Paint tool.\nReimporting it will override any changes.\nWould you like to continue?"),
			FText::FromString(pTex2D->GetName()))))
	{
		return EReimportResult::Failed;
	}

	UE_LOG(LogHL2Editor, Log, TEXT("Performing atomic reimport of [%s]"), *ResolvedSourceFilePath);

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		UE_LOG(LogHL2Editor, Warning, TEXT("-- cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	// We use this reimport factory to skip the object creation process
	// which obliterates all of the properties of the texture.
	// Also preset the factory with the settings of the current texture.
	// These will be used during the import and compression process.        
	CompressionSettings = pTex->CompressionSettings;
	NoCompression = pTex->CompressionNone;
	NoAlpha = pTex->CompressionNoAlpha;
	bDeferCompression = pTex->DeferCompression;
	MipGenSettings = pTex->MipGenSettings;

	float Brightness = 0.f;
	float TextureMultiplier = 1.f;

	UTextureLightProfile* pTexLightProfile = Cast<UTextureLightProfile>(Obj);
	if (pTexLightProfile)
	{
		Brightness = pTexLightProfile->Brightness;
		TextureMultiplier = pTexLightProfile->TextureMultiplier;
	}

	// Suppress the import overwrite dialog because we know that for explicitly re-importing we want to preserve existing settings
	//UVTFFactory::SuppressImportOverwriteDialog();

	bool OutCanceled = false;

	if (ImportObject(pTex->GetClass(), pTex->GetOuter(), *pTex->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{
		if (pTexLightProfile)
		{
			// We don't update the Brightness and TextureMultiplier during reimport.
			// The reason is that the IESLoader has changed and calculates these values differently.
			// Since existing lights have been calibrated, we don't want to screw with those values.
			pTexLightProfile->Brightness = Brightness;
			pTexLightProfile->TextureMultiplier = TextureMultiplier;
		}

		UE_LOG(LogHL2Editor, Log, TEXT("-- imported successfully"));

		pTex->AssetImportData->Update(ResolvedSourceFilePath);

		// Try to find the outer package so we can dirty it up
		if (pTex->GetOuter())
		{
			pTex->GetOuter()->MarkPackageDirty();
		}
		else
		{
			pTex->MarkPackageDirty();
		}
	}
	else if (OutCanceled)
	{
		UE_LOG(LogHL2Editor, Warning, TEXT("-- import canceled"));
		return EReimportResult::Cancelled;
	}
	else
	{
		UE_LOG(LogHL2Editor, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}

	return EReimportResult::Succeeded;
}

int32 UReimportVTFFactory::GetPriority() const
{
	return ImportPriority;
}