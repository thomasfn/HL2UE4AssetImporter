#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "EntityParser.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2EntityParser, Log, All);

USTRUCT(BlueprintType)
struct FHL2Entity
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

enum class EEntityParserTokenType
{
	Whitespace,
	OpenGroup,
	CloseGroup,
	String,
	ETokenType_Count
};

struct FEntityParserToken
{
	EEntityParserTokenType type;
	int start;
	int end;
};

class FEntityParser
{
private:

	FEntityParser();

private:

	static const FString entityParserTokenTypeNames[(int)EEntityParserTokenType::ETokenType_Count];
	
public:

	/**
	 * Parses a string from a vbsp entity lump into an array of entities.
	 */
	static bool ParseEntities(const FString& src, TArray<FHL2Entity>& out);

private:

	static bool Tokenise(const FString& src, TArray<FEntityParserToken>& out);

	static void ReadToken(const FString& src, const FEntityParserToken& token, FString& out);

	static bool ParseGroup(const FString& src, const TArray<FEntityParserToken>& tokens, int& nextToken, TMap<FName, FString>& out);

	inline static void ParseCommonKeys(FHL2Entity& entity);
};
