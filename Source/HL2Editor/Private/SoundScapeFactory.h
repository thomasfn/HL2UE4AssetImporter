#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "HL2SoundScripts.h"
#include "ValveKeyValues.h"

#include "SoundScapeFactory.generated.h"

UCLASS()
class USoundScapeFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoundScapeFactory();
	virtual ~USoundScapeFactory();

	// Begin UFactory Interface

	virtual UObject* FactoryCreateText(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const TCHAR*& Buffer,
		const TCHAR* BufferEnd,
		FFeedbackContext* Warn
	) override;

	virtual bool DoesSupportClass(UClass* Class) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	// End UFactory Interface


	virtual UHL2SoundScapes* CreateSoundScapes(UObject* InParent, FName Name, EObjectFlags Flags);

private:

	UHL2SoundScapes* ImportSoundScapes(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);

	void ParseDocumentToEntries(const UValveDocument* Document, TMap<FName, FHL2SoundScape>& OutEntries, FFeedbackContext* Warn);

	void ParseSoundScapeEntry(const UValveGroupValue* GroupValue, FHL2SoundScape& OutEntry);

};
