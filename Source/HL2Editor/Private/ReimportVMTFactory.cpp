#include "HL2EditorPrivatePCH.h"

#include "ReimportVMTFactory.h"
#include "EditorFramework/AssetImportData.h"

UReimportVMTFactory::UReimportVMTFactory()
{
	SupportedClass = UVMTMaterial::StaticClass();

	bCreateNew = false;
}

UReimportVMTFactory::~UReimportVMTFactory()
{
	
}

UVMTMaterial* UReimportVMTFactory::CreateMaterial(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UVMTMaterial* pMat = Cast<UVMTMaterial>(pOriginalMat);

	if (pMat)
	{
		return pMat;
	}
	else
	{
		return Super::CreateMaterial(InParent, Name, Flags);
	}
}

bool UReimportVMTFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UVMTMaterial* pMat = Cast<UVMTMaterial>(Obj);
	if (pMat && pMat->AssetImportData)
	{
		TArray<FString> fileNames = pMat->AssetImportData->ExtractFilenames();
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

void UReimportVMTFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UVMTMaterial* pMat = Cast<UVMTMaterial>(Obj);
	if (pMat && ensure(NewReimportPaths.Num() == 1))
	{
		pMat->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

/**
* Reimports specified texture from its source material, if the meta-data exists
*/
EReimportResult::Type UReimportVMTFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UVMTMaterial::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UVMTMaterial* pMat = Cast<UVMTMaterial>(Obj);

	TGuardValue<UVMTMaterial*> OriginalTexGuardValue(pOriginalMat, pMat);

	const FString ResolvedSourceFilePath = pMat->AssetImportData->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		// Since this is a new system most textures don't have paths, so logging has been commented out
		//UE_LOG(LogHL2Editor, Warning, TEXT("-- cannot reimport: texture resource does not have path stored."));
		return EReimportResult::Failed;
	}

	UE_LOG(LogHL2Editor, Log, TEXT("Performing atomic reimport of [%s]"), *ResolvedSourceFilePath);

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		UE_LOG(LogHL2Editor, Warning, TEXT("-- cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	bool OutCanceled = false;

	if (ImportObject(pMat->GetClass(), pMat->GetOuter(), *pMat->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogHL2Editor, Log, TEXT("-- imported successfully"));

		pMat->AssetImportData->Update(ResolvedSourceFilePath);

		// Try to find the outer package so we can dirty it up
		if (pMat->GetOuter())
		{
			pMat->GetOuter()->MarkPackageDirty();
		}
		else
		{
			pMat->MarkPackageDirty();
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

int32 UReimportVMTFactory::GetPriority() const
{
	return ImportPriority;
}