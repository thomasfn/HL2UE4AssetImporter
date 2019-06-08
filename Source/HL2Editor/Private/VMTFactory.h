#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "VMTMaterial.h"

#include "VMTFactory.generated.h"

UCLASS()
class UVMTFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVMTFactory();
	virtual ~UVMTFactory();

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

	/** Create a material given the appropriate input parameters	*/
	virtual UVMTMaterial* CreateMaterial(UObject* InParent, FName Name, EObjectFlags Flags);

private:

	UVMTMaterial* ImportMaterial(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);

};