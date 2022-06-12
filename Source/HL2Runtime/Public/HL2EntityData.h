#pragma once

#include "CoreMinimal.h"

#include "HL2EntityData.generated.h"

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FEntityLogicOutput
{
	GENERATED_BODY()

public:

	// The targetname of the target. May be a wildcard or special targetname.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FName TargetName;

	// The name of the output.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FName OutputName;

	// The name of the input of the target to fire.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FName InputName;

	// How long to wait before firing (s).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	float Delay;

	// Whether to only fire once.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	bool Once;

	// Parameters.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FString> Params;
};

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FHL2EntityData
{

	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<FName, FString> KeyValues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FEntityLogicOutput> LogicOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FName Classname = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FString Targetname = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FVector3f Origin = FVector3f::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	int SpawnFlags = 0;

public:

	FString GetString(FName key) const;

	bool TryGetString(FName key, FString& out) const;

	int GetInt(FName key) const;

	bool TryGetInt(FName key, int& out) const;

	bool GetBool(FName key) const;

	bool TryGetBool(FName key, bool& out) const;

	float GetFloat(FName key) const;

	bool TryGetFloat(FName key, float& out) const;

	FVector3f GetVector(FName key) const;

	bool TryGetVector(FName key, FVector3f& out) const;

	FVector4f GetVector4(FName key) const;

	bool TryGetVector4(FName key, FVector4f& out) const;

};