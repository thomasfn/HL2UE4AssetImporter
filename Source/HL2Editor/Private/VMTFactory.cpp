#include "HL2EditorPrivatePCH.h"

#include "VMTFactory.h"

#include "Runtime/Core/Public/Misc/FeedbackContext.h"

#include "VTFLib/VTFLib.h"

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

	UVMTMaterial* material = ImportMaterial(InClass, InParent, InName, Flags, Type, Buffer, BufferEnd, Warn);

	if (!material)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Material import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
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

UVMTMaterial* UVMTFactory::CreateMaterial(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UVMTMaterial::StaticClass(), InParent, Name, Flags);
	UVMTMaterial* newMaterial = nullptr;
	if (newObject)
	{
		newMaterial = CastChecked<UVMTMaterial>(newObject);
	}

	return newMaterial;
}

UVMTMaterial* UVMTFactory::ImportMaterial(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	// Convert to ANSI string
	const auto data = StringCast<ANSICHAR, TCHAR>(Buffer, BufferEnd - Buffer);

	// Throw it at VTFLib
	VTFLib::CVMTFile vmtFile;
	if (!vmtFile.Load(data.Get(), data.Length()))
	{
		FString err = VTFLib::LastError.Get();
		Warn->Logf(ELogVerbosity::Error, TEXT("Failed to load VMT: %s"), *err);
		return nullptr;
	}

	// Create a new material
	UVMTMaterial* material = CreateMaterial(InParent, Name, Flags);

	// Attempt to import it
	material->SetFromVMT(*vmtFile.GetRoot());

	return material;
}