#include "ReimportMDLFactory.h"

#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"

UReimportMDLFactory::UReimportMDLFactory()
{
	SupportedClass = UStaticMesh::StaticClass();

	bCreateNew = false;

	++ImportPriority;
}

UReimportMDLFactory::~UReimportMDLFactory()
{
	
}

UStaticMesh* UReimportMDLFactory::CreateStaticMesh(UObject* InParent, FName Name, EObjectFlags Flags)
{
	if (pOriginalStaticMesh)
	{
		return pOriginalStaticMesh;
	}
	else
	{
		return Super::CreateStaticMesh(InParent, Name, Flags);
	}
}

USkeletalMesh* UReimportMDLFactory::CreateSkeletalMesh(UObject* InParent, FName Name, EObjectFlags Flags)
{
	if (pOriginalSkeletalMesh)
	{
		return pOriginalSkeletalMesh;
	}
	else
	{
		return Super::CreateSkeletalMesh(InParent, Name, Flags);
	}
}

USkeleton* UReimportMDLFactory::CreateSkeleton(UObject* InParent, FName Name, EObjectFlags Flags)
{
	if (pOriginalSkeleton)
	{
		return pOriginalSkeleton;
	}
	else
	{
		return Super::CreateSkeleton(InParent, Name, Flags);
	}
}

UPhysicsAsset* UReimportMDLFactory::CreatePhysAsset(UObject* InParent, FName Name, EObjectFlags Flags)
{
	if (pOriginalPhysicsAsset)
	{
		return pOriginalPhysicsAsset;
	}
	else
	{
		return Super::CreatePhysAsset(InParent, Name, Flags);
	}
}

UAnimSequence* UReimportMDLFactory::CreateAnimSequence(UObject* InParent, FName Name, EObjectFlags Flags)
{
	if (pOriginalAnimSequence)
	{
		return pOriginalAnimSequence;
	}
	else
	{
		return Super::CreateAnimSequence(InParent, Name, Flags);
	}
}

bool UReimportMDLFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	TArray<FString> fileNames;
	UStaticMesh* pStaticMesh = Cast<UStaticMesh>(Obj);
	if (pStaticMesh && pStaticMesh->AssetImportData)
	{
		fileNames = pStaticMesh->AssetImportData->ExtractFilenames();
	}
	USkeletalMesh* pSkeletalMesh = Cast<USkeletalMesh>(Obj);
	if (pSkeletalMesh && pSkeletalMesh->AssetImportData)
	{
		fileNames = pSkeletalMesh->AssetImportData->ExtractFilenames();
	}
	if (fileNames.Num() == 0)
	{
		return false;
	}
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

void UReimportMDLFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UStaticMesh* pStaticMesh = Cast<UStaticMesh>(Obj);
	if (pStaticMesh && ensure(NewReimportPaths.Num() == 1))
	{
		pStaticMesh->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
	USkeletalMesh* pSkeletalMesh = Cast<USkeletalMesh>(Obj);
	if (pSkeletalMesh && ensure(NewReimportPaths.Num() == 1))
	{
		pSkeletalMesh->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

/**
* Reimports specified model from its source file, if the meta-data exists
*/
EReimportResult::Type UReimportMDLFactory::Reimport(UObject* Obj)
{
	if (!Obj || (
		!Obj->IsA(UStaticMesh::StaticClass()) &&
		!Obj->IsA(USkeletalMesh::StaticClass()) &&
		!Obj->IsA(USkeleton::StaticClass()) &&
		!Obj->IsA(UPhysicsAsset::StaticClass()) &&
		!Obj->IsA(UAnimSequence::StaticClass())
	))
	{
		return EReimportResult::Failed;
	}

	UStaticMesh* pStaticMesh = Cast<UStaticMesh>(Obj);
	USkeletalMesh* pSkeletalMesh = Cast<USkeletalMesh>(Obj);

	TGuardValue<UStaticMesh*> OriginalStaticMeshGuardValue(pOriginalStaticMesh, pStaticMesh);
	TGuardValue<USkeletalMesh*> OriginalSkeletalMeshGuardValue(pOriginalSkeletalMesh, pSkeletalMesh);

	UObject* mesh = nullptr;
	UAssetImportData* assetImportData = nullptr;
	if (pStaticMesh != nullptr)
	{
		assetImportData = pStaticMesh->AssetImportData;
		mesh = pStaticMesh;
	}
	else if (pSkeletalMesh != nullptr)
	{
		assetImportData = pSkeletalMesh->AssetImportData;
		mesh = pSkeletalMesh;
	}
	check(mesh);
	check(assetImportData);

	const FString ResolvedSourceFilePath = assetImportData->GetFirstFilename();
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

	if (ImportObject(mesh->GetClass(), mesh->GetOuter(), *mesh->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogHL2Editor, Log, TEXT("-- imported successfully"));

		assetImportData->Update(ResolvedSourceFilePath);

		// Try to find the outer package so we can dirty it up
		if (mesh->GetOuter())
		{
			mesh->GetOuter()->MarkPackageDirty();
		}
		else
		{
			mesh->MarkPackageDirty();
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

int32 UReimportMDLFactory::GetPriority() const
{
	return ImportPriority;
}