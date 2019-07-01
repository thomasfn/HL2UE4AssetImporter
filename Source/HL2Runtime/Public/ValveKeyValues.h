#pragma once

#include "CoreMinimal.h"
#include "Object.h"

#include "ValveKeyValues.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogValveKeyValuesParser, Log, All);

UCLASS()
class HL2RUNTIME_API UValveValue : public UObject
{
	GENERATED_BODY()

public:

};

class UValveGroupValue;
class UValveArrayValue;
class UValvePrimitiveValue;

UCLASS()
class HL2RUNTIME_API UValveComplexValue : public UValveValue
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual UValveValue* GetValue(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual UValvePrimitiveValue* GetPrimitive(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual UValveGroupValue* GetGroup(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual UValveArrayValue* GetArray(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual bool GetInt(FName path, int& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual bool GetFloat(FName path, float& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual bool GetBool(FName path, bool& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual bool GetString(FName path, FString& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Complex")
	virtual bool GetName(FName path, FName& outValue) const;
};

UCLASS()
class HL2RUNTIME_API UValveArrayValue : public UValveComplexValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Array")
	TArray<UValveValue*> Items;

};

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FValveGroupKeyValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Group")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Group")
	UValveValue* Value;
};

UCLASS()
class HL2RUNTIME_API UValveGroupValue : public UValveComplexValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Group")
	TArray<FValveGroupKeyValue> Items;

public:

	UFUNCTION(BlueprintCallable)
	UValveValue* GetItem(FName key) const;

	UFUNCTION(BlueprintCallable)
	int GetItems(FName key, TArray<UValveValue*>& outItems) const;

};

UCLASS()
class HL2RUNTIME_API UValvePrimitiveValue : public UValveValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Primitive")
	FString Value;

public:

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Primitive")
	virtual float AsFloat(float defaultValue = 0.0f) const;
	
	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Primitive")
	virtual int AsInt(int defaultValue = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Primitive")
	virtual bool AsBool(bool defaultValue = false) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Primitive")
	virtual FString AsString() const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Primitive")
	virtual FName AsName() const;
};

UCLASS()
class HL2RUNTIME_API UValveDocument : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Document")
	UValveComplexValue* Root;

public:

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	static UValveDocument* Parse(const FString& text, UObject* outer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	UValveValue* GetValue(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	bool GetInt(FName path, int& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	bool GetFloat(FName path, float& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	bool GetBool(FName path, bool& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	bool GetString(FName path, FString& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	bool GetName(FName path, FName& outValue) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	UValveGroupValue* GetGroup(FName path) const;

	UFUNCTION(BlueprintCallable, Category = "Valve Key Values - Document")
	UValveArrayValue* GetArray(FName path) const;
};