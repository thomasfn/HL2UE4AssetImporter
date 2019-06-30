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

UCLASS()
class HL2RUNTIME_API UValveArrayValue : public UValveValue
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
class HL2RUNTIME_API UValveGroupValue : public UValveValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Group")
	TArray<FValveGroupKeyValue> Items;

public:

	UFUNCTION(BlueprintCallable)
	UValveValue* GetItem(FName key);

	UFUNCTION(BlueprintCallable)
	int GetItems(FName key, TArray<UValveValue*>& outItems);

};

UCLASS()
class HL2RUNTIME_API UValvePrimitiveValue : public UValveValue
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	virtual float AsFloat(float defaultValue = 0.0f) const;
	
	UFUNCTION(BlueprintCallable)
	virtual int AsInt(int defaultValue = 0) const;

	UFUNCTION(BlueprintCallable)
	virtual bool AsBool(bool defaultValue = false) const;

	UFUNCTION(BlueprintCallable)
	virtual FString AsString() const;

	UFUNCTION(BlueprintCallable)
	virtual FName AsName() const;
};

UCLASS()
class HL2RUNTIME_API UValveIntegerValue : public UValvePrimitiveValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Integer")
	int32 Value;

	virtual float AsFloat(float defaultValue = 0.0f) const override;

	virtual int AsInt(int defaultValue = 0) const override;

	virtual bool AsBool(bool defaultValue = false) const override;

	virtual FString AsString() const override;
};

UCLASS()
class HL2RUNTIME_API UValveFloatValue : public UValvePrimitiveValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Float")
	float Value;

public:

	virtual float AsFloat(float defaultValue = 0.0f) const override;

	virtual int AsInt(int defaultValue = 0) const override;

	virtual bool AsBool(bool defaultValue = false) const override;

	virtual FString AsString() const override;
};

UCLASS()
class HL2RUNTIME_API UValveStringValue : public UValvePrimitiveValue
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - String")
	FString Value;

public:

	virtual float AsFloat(float defaultValue = 0.0f) const override;

	virtual int AsInt(int defaultValue = 0) const override;

	virtual bool AsBool(bool defaultValue = false) const override;

	virtual FString AsString() const override;
};

UCLASS()
class HL2RUNTIME_API UValveDocument : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valve Key Values - Document")
	UValveValue* Root;

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