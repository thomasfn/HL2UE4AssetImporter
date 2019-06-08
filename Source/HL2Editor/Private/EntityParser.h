#pragma once

#include "CoreMinimal.h"
#include "HL2EntityData.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2EntityParser, Log, All);

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
	static bool ParseEntities(const FString& src, TArray<FHL2EntityData>& out);

private:

	static bool Tokenise(const FString& src, TArray<FEntityParserToken>& out);

	static void ReadToken(const FString& src, const FEntityParserToken& token, FString& out);

	static bool ParseGroup(const FString& src, const TArray<FEntityParserToken>& tokens, int& nextToken, TMap<FName, FString>& out);

	inline static void ParseCommonKeys(FHL2EntityData& entity);
};
