#include "SoundScapeFactory.h"
#include "SoundScriptUtils.h"
#include "Runtime/Core/Public/Misc/FeedbackContext.h"
#include "IHL2Editor.h"

USoundScapeFactory::USoundScapeFactory()
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
	SupportedClass = UHL2SoundScapes::StaticClass();
}

USoundScapeFactory::~USoundScapeFactory()
{
	
}

// Begin UFactory Interface

UObject* USoundScapeFactory::FactoryCreateText(
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

	UHL2SoundScapes* soundScapes = ImportSoundScapes(InClass, InParent, InName, Flags, Type, Buffer, BufferEnd, Warn);

	if (!soundScapes)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Soundscape import failed"));
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, soundScapes);

	soundScapes->PostEditChange();

	return soundScapes;
}

/** Returns whether or not the given class is supported by this factory. */
bool USoundScapeFactory::DoesSupportClass(UClass* Class)
{
	return Class == UHL2SoundScapes::StaticClass();
}

/** Returns true if this factory can deal with the file sent in. */
bool USoundScapeFactory::FactoryCanImport(const FString& Filename)
{
	FString Extension = FPaths::GetExtension(Filename);
	return Extension.Compare("txt", ESearchCase::IgnoreCase) == 0;
}

// End UFactory Interface

UHL2SoundScapes* USoundScapeFactory::CreateSoundScapes(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UObject* newObject = CreateOrOverwriteAsset(UHL2SoundScapes::StaticClass(), InParent, Name, Flags);
	UHL2SoundScapes* newSoundScapes = nullptr;
	if (newObject)
	{
		newSoundScapes = CastChecked<UHL2SoundScapes>(newObject);
	}

	return newSoundScapes;
}

UHL2SoundScapes* USoundScapeFactory::ImportSoundScapes(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
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
	UHL2SoundScapes* soundScapes = CreateSoundScapes(InParent, Name, Flags);

	// Fetch manifest
	static const FName fnSoundScaplesManifest(TEXT("soundscaples_manifest"));
	static const FName fnFile(TEXT("file"));
	const UValveGroupValue* rootGroupValue = CastChecked<UValveGroupValue>(document->Root);
	const UValveValue* soundScaplesManifestValue = rootGroupValue->GetItem(fnSoundScaplesManifest);
	if (soundScaplesManifestValue == nullptr)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Sound script was not a soundscape manifest"));
		return nullptr;
	}
	const UValveGroupValue* soundScaplesManifest = CastChecked<UValveGroupValue>(soundScaplesManifestValue);
	FString path = FPaths::GetPath(CurrentFilename);
	for (const FValveGroupKeyValue& keyValue : soundScaplesManifest->Items)
	{
		if (keyValue.Key != fnFile) { continue; }
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
			Warn->Logf(ELogVerbosity::Error, TEXT("Failed to parse soundscapes"));
			return nullptr;
		}
		ParseDocumentToEntries(scriptDocument, soundScapes->Entries, Warn);
		scriptDocument->MarkAsGarbage();
	}

	// Clean up
	document->MarkAsGarbage();

	return soundScapes;
}

void USoundScapeFactory::ParseDocumentToEntries(const UValveDocument* Document, TMap<FName, FHL2SoundScape>& OutEntries, FFeedbackContext* Warn)
{
	const UValveGroupValue* scriptGroup = CastChecked<UValveGroupValue>(Document->Root);
	for (const FValveGroupKeyValue& keyValue : scriptGroup->Items)
	{
		const FName soundScapeName = keyValue.Key;
		const UValveGroupValue* entryGroup = CastChecked<UValveGroupValue>(keyValue.Value);
		FHL2SoundScape soundScape;
		soundScape.SoundscapeName = soundScapeName;
		ParseSoundScapeEntry(entryGroup, soundScape);
		OutEntries.Add(soundScapeName, soundScape);
	}
}

void USoundScapeFactory::ParseSoundScapeEntry(const UValveGroupValue* GroupValue, FHL2SoundScape& OutEntry)
{
	static const FName fnPlayLooping(TEXT("playlooping"));
	static const FName fnPlayRandom(TEXT("playrandom"));
	static const FName fnPlaySoundScape(TEXT("playsoundscape"));
	static const FName fnValveDSP(TEXT("dsp"));
	static const FName fnValveDSPVolume(TEXT("dsp_volume"));
	static const FName fnValveVolume(TEXT("volume"));
	static const FName fnValvePosition(TEXT("position"));
	static const FName fnValveName(TEXT("name"));

	ParseGenericNumber<uint8>(GroupValue->GetPrimitive(fnValveDSP), OutEntry.DSPEffect, 0);
	ParseGenericNumber<float>(GroupValue->GetPrimitive(fnValveDSPVolume), OutEntry.DSPVolume, 1.0f);
	for (const FValveGroupKeyValue& keyValue : GroupValue->Items)
	{
		if (keyValue.Key == fnPlayLooping)
		{
			const UValveGroupValue* ruleGroup = CastChecked<UValveGroupValue>(keyValue.Value);
			FHL2SoundScapeRule rule;
			rule.RuleType = EHL2SoundScapeRuleType::PlayLooping;
			ParseGenericNumber<uint8>(ruleGroup->GetPrimitive(fnValvePosition), rule.Position, 0);
			FSoundScriptUtils::ParseSoundScriptEntry(ruleGroup, rule.SoundScript);
			OutEntry.Rules.Add(rule);
		}
		else if (keyValue.Key == fnPlayRandom)
		{
			const UValveGroupValue* ruleGroup = CastChecked<UValveGroupValue>(keyValue.Value);
			FHL2SoundScapeRule rule;
			rule.RuleType = EHL2SoundScapeRuleType::PlayRandom;
			ParseGenericNumber<uint8>(ruleGroup->GetPrimitive(fnValvePosition), rule.Position, 0);
			FSoundScriptUtils::ParseSoundScriptEntry(ruleGroup, rule.SoundScript);
			OutEntry.Rules.Add(rule);
		}
		else if (keyValue.Key == fnPlaySoundScape)
		{
			const UValveGroupValue* ruleGroup = CastChecked<UValveGroupValue>(keyValue.Value);
			const UValvePrimitiveValue* nameValue = ruleGroup->GetPrimitive(fnValveName);
			if (nameValue == nullptr) { continue; }
			FHL2SoundScapeSubSoundScapeRule rule;
			ParseGenericNumber<float>(ruleGroup->GetPrimitive(fnValveVolume), rule.Volume, 1.0f);
			ParseGenericNumber<uint8>(ruleGroup->GetPrimitive(fnValvePosition), rule.Position, 0);
			rule.SubSoundScapeName = FName(*nameValue->AsString());
			OutEntry.SubSoundScapeRules.Add(rule);
		}
	}
}
