#pragma once

#include "CoreMinimal.h"

#include "HL2EntityData.generated.h"

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FHL2EntityData
{

	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<FName, FString> KeyValues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FName Classname = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FString Targetname = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FVector Origin = FVector::ZeroVector;

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

	FVector GetVector(FName key) const;

	bool TryGetVector(FName key, FVector& out) const;

	FRotator GetRotator(FName key) const;

	bool TryGetRotator(FName key, FRotator& out) const;

};