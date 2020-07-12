#include "VMTFactory.h"
#include "MaterialUtils.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "ValveKeyValues.h"
#include "IHL2Editor.h"

UVMTFactory::UVMTFactory()
{
	// We only want to create assets by importing files.
	// Set this to true if you want to be able to create new, empty Assets from
	// the editor.
	bCreateNew = false;

	// Our Asset will be imported from a text file
	bText = true;

	// Allows us to actually use the "Import" button in the Editor for this Asset
	bEditorImport = true;

	// Tells the Editor which file types we can import
	Formats.Add(TEXT("vmt;Valve Material Type Files"));

	// Tells the Editor which Asset type this UFactory can import
	SupportedClass = UVMTMaterial::StaticClass();
}

UVMTFactory::~UVMTFactory()
{
	
}

// Begin UFactory Interface

UObject* UVMTFactory::FactoryCreateText(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const TCHAR*& Buffer,
	const TCHAR* BufferEnd,
	FFeedbackContext* Warn
)
{
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);

	UMaterialInterface* material = ImportMaterial(InClass, InParent, InName, Flags, Type, Buffer, BufferEnd, Warn);

	if (!material)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Material import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	if (material->AssetImportData == nullptr)
	{
		material->AssetImportData = NewObject<UAssetImportData>(material);
	}
	material->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, material);

	material->PostEditChange();

	return material;
}

/** Returns whether or not the given class is supported by this factory. */
bool UVMTFactory::DoesSupportClass(UClass* Class)
{
	return Class == UVMTMaterial::StaticClass();
}

/** Returns true if this factory can deal with the file sent in. */
bool UVMTFactory::FactoryCanImport(const FString& Filename)
{
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Compare("vmt", ESearchCase::IgnoreCase) == 0;
}

// End UFactory Interface

UMaterialInterface* UVMTFactory::CreateMaterial(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UClass* materialClass = IHL2Editor::Get().GetConfig().Material.Portable ? UMaterialInstanceConstant::StaticClass() : UVMTMaterial::StaticClass();
	UObject* newObject = CreateOrOverwriteAsset(materialClass, InParent, Name, Flags);
	UMaterialInterface* newMaterial = nullptr;
	if (newObject)
	{
		newMaterial = CastChecked<UMaterialInterface>(newObject);
	}

	return newMaterial;
}

UMaterialInterface* UVMTFactory::ImportMaterial(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	// Convert to string
	const FString text(BufferEnd - Buffer, Buffer);

	// Parse to a document
	UValveDocument* document = UValveDocument::Parse(text, this);
	if (document == nullptr)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Failed to parse VMT"));
		return nullptr;
	}

	// Create a new material
	UMaterialInterface* material = CreateMaterial(InParent, Name, Flags);

	// Attempt to import it
	if (!FMaterialUtils::SetFromVMT(CastChecked<UMaterialInstanceConstant>(material), document))
	{
		//Warn->Logf(ELogVerbosity::Error, TEXT("Failed to import VMT"));
		//return nullptr;
	}

	// Clean up
	document->MarkPendingKill();

	return material;
}