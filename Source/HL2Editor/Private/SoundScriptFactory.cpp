#include "SoundScriptFactory.h"
#include "SoundScriptUtils.h"
#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "IHL2Editor.h"

USoundScriptFactory::USoundScriptFactory()
{
	// We only want to create assets by importing files.
	// Set this to true if you want to be able to create new, empty Assets from
	// the editor.
	bCreateNew = false;

	// Our Asset will be imported from a text file
	bText = true;

	// Allows us to actually use the "Import" button in the Editor for this Asset
	bEditorImport = false;

	// Tells the Editor which file types we can import
	Formats.Add(TEXT("txt;Valve Script Files"));

	// Tells the Editor which Asset type this UFactory can import
	SupportedClass = UHL2SoundScripts::StaticClass();
}

USoundScriptFactory::~USoundScriptFactory()
{
	
}

// Begin UFactory Interface

UObject* USoundScriptFactory::FactoryCreateText(
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

	UHL2SoundScripts* soundScripts = ImportSoundScripts(InClass, InParent, InName, Flags, Type, Buffer, BufferEnd, Warn);

	if (!soundScripts)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Sound scripts import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	/*if (soundScripts->AssetImportData == nullptr)
	{
		soundScripts->AssetImportData = NewObject<UAssetImportData>(soundScripts);
	}
	soundScripts->AssetImportData->Update(CurrentFilename, FileHash.IsValid() ? &FileHash : nullptr);*/

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, soundScripts);

	soundScripts->PostEditChange();

	return soundScripts;
}

/** Returns whether or not the given class is supported by this factory. */
bool USoundScriptFactory::DoesSupportClass(UClass* Class)
{
	return Class == UHL2SoundScripts::StaticClass();
}

/** Returns true if this factory can deal with the file sent in. */
bool USoundScriptFactory::FactoryCanImport(const FString& Filename)
{
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Compare("txt", ESearchCase::IgnoreCase) == 0;
}

// End UFactory Interface

UHL2SoundScripts* USoundScriptFactory::CreateSoundScripts(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UHL2SoundScripts::StaticClass(), InParent, Name, Flags);
	UHL2SoundScripts* newSoundScripts = nullptr;
	if (newObject)
	{
		newSoundScripts = CastChecked<UHL2SoundScripts>(newObject);
	}

	return newSoundScripts;
}

UHL2SoundScripts* USoundScriptFactory::ImportSoundScripts(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	// Convert to string
	const FString text(BufferEnd - Buffer, Buffer);

	// Parse to a document
	UValveDocument* document = UValveDocument::Parse(text, this);
	if (document == nullptr)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Failed to parse scripts"));
		return nullptr;
	}

	// Create a new sound scripts asset
	UHL2SoundScripts* soundScripts = CreateSoundScripts(InParent, Name, Flags);

	// Fetch manifest
	static const FName fnGameSoundsManifest(TEXT("game_sounds_manifest"));
	static const FName fnPrecacheFile(TEXT("precache_file"));
	const UValveGroupValue* rootGroupValue = CastChecked<UValveGroupValue>(document->Root);
	const UValveValue* gameSoundsManifestValue = rootGroupValue->GetItem(fnGameSoundsManifest);
	if (gameSoundsManifestValue == nullptr)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Sound script was not a manifest"));
		return nullptr;
	}
	const UValveGroupValue* gameSoundsManifest = CastChecked<UValveGroupValue>(gameSoundsManifestValue);
	FString path = FPaths::GetPath(CurrentFilename);
	for (const FValveGroupKeyValue& keyValue : gameSoundsManifest->Items)
	{
		if (keyValue.Key != fnPrecacheFile) { continue; }
		const UValvePrimitiveValue* value = CastChecked<UValvePrimitiveValue>(keyValue.Value);
		const FString scriptPath = path / TEXT("..") / value->AsString();
		FString scriptText;
		if (!FFileHelper::LoadFileToString(scriptText, *scriptPath))
		{
			Warn->Logf(ELogVerbosity::Warning, TEXT("Failed to load '%s'"), *scriptPath);
			continue;
		}
		UValveDocument* scriptDocument = UValveDocument::Parse(scriptText, this);
		if (scriptDocument == nullptr)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("Failed to parse scripts"));
			return nullptr;
		}
		ParseDocumentToEntries(scriptDocument, soundScripts->Entries, Warn);
		scriptDocument->MarkAsGarbage();
	}

	// Clean up
	document->MarkAsGarbage();

	return soundScripts;
}

void USoundScriptFactory::ParseDocumentToEntries(const UValveDocument* Document, TMap<FName, FHL2SoundScriptEntry>& OutEntries, FFeedbackContext* Warn)
{
	const UValveGroupValue* scriptGroup = CastChecked<UValveGroupValue>(Document->Root);
	for (const FValveGroupKeyValue& keyValue : scriptGroup->Items)
	{
		const FName entryName = keyValue.Key;
		const UValveGroupValue* entryGroup = CastChecked<UValveGroupValue>(keyValue.Value);
		FHL2SoundScriptEntry entry;
		entry.EntryName = entryName;
		FSoundScriptUtils::ParseSoundScriptEntry(entryGroup, entry);
		OutEntries.Add(entryName, entry);
	}
}
