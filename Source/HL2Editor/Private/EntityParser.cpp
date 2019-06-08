#pragma once

#include "EntityParser.h"

#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY(LogHL2EntityParser);

const FString FEntityParser::entityParserTokenTypeNames[] =
{
	TEXT("Whitespace"),
	TEXT("OpenGroup"),
	TEXT("CloseGroup"),
	TEXT("String")
};

bool FEntityParser::Tokenise(const FString& src, TArray<FEntityParserToken>& out)
{
	const static FRegexPattern patterns[] =
	{
		FRegexPattern(TEXT("[\\s\\n\\r]+")), // Whitespace
		FRegexPattern(TEXT("\\{")), // OpenGroup
		FRegexPattern(TEXT("\\}")), // CloseGroup
		FRegexPattern(TEXT("\"((?:\\\\\"|[^\"])*)\"")) // String
	};
	constexpr int numPatterns = sizeof(patterns) / sizeof(FRegexPattern);
	static_assert(numPatterns == (int)EEntityParserTokenType::ETokenType_Count, "EntityParser.cpp: Each token must have a regex pattern");
	int curPos = 0;
	while (curPos < src.Len())
	{
		bool matched = false;
		for (int i = 0; i < numPatterns; ++i)
		{
			FRegexMatcher matcher(patterns[i], src);
			matcher.SetLimits(curPos, src.Len());
			if (matcher.FindNext() && matcher.GetMatchBeginning() == curPos)
			{
				matched = true;
				if (i > 0) // ignore whitespace
				{
					FEntityParserToken token;
					token.type = (EEntityParserTokenType)i;
					token.start = curPos;
					token.end = matcher.GetMatchEnding();
					out.Add(token);
				}
				curPos = matcher.GetMatchEnding();
				break;
			}
		}
		if (!matched)
		{
			UE_LOG(LogHL2EntityParser, Error, TEXT("Unexpected '%c' at %d"), src[curPos], curPos);
			return false;
		}
	}
	return true;
}

void FEntityParser::ReadToken(const FString& src, const FEntityParserToken& token, FString& out)
{
	out = FString(token.end - token.start, *src + token.start);
}

bool FEntityParser::ParseGroup(const FString& src, const TArray<FEntityParserToken>& tokens, int& nextToken, TMap<FName, FString>& out)
{
	// OpenGroup
	if (tokens[nextToken++].type != EEntityParserTokenType::OpenGroup) { return false; }

	// String | CloseGroup
	while (tokens[nextToken].type != EEntityParserTokenType::CloseGroup)
	{
		// String
		const FEntityParserToken& keyToken = tokens[nextToken++];
		if (keyToken.type != EEntityParserTokenType::String)
		{
			UE_LOG(LogHL2EntityParser, Error, TEXT("Expected 'String', got '%s' at %d"), *entityParserTokenTypeNames[(int)keyToken.type], keyToken.start);
			return false;
		}

		// String
		const FEntityParserToken& valueToken = tokens[nextToken++];
		if (valueToken.type != EEntityParserTokenType::String)
		{
			UE_LOG(LogHL2EntityParser, Error, TEXT("Expected 'String', got '%s' at %d"), *entityParserTokenTypeNames[(int)valueToken.type], valueToken.start);
			return false;
		}

		FString keyStr, valueStr;
		ReadToken(src, keyToken, keyStr);
		keyStr.RemoveFromStart(TEXT("\""));
		keyStr.RemoveFromEnd(TEXT("\""));
		ReadToken(src, valueToken, valueStr);
		valueStr.RemoveFromStart(TEXT("\""));
		valueStr.RemoveFromEnd(TEXT("\""));
		out.Add(FName(*keyStr), valueStr);
	}

	// CloseGroup
	nextToken++;

	return true;
}

bool FEntityParser::ParseEntities(const FString& src, TArray<FHL2EntityData>& out)
{
	TArray<FEntityParserToken> tokens;
	if (!Tokenise(src, tokens))
	{
		UE_LOG(LogHL2EntityParser, Error, TEXT("Failed to tokenise entity string"));
		return false;
	}
	TMap<FName, FString> keyValues;
	int nextToken = 0;
	while (nextToken < tokens.Num())
	{
		int tmp = nextToken;
		if (ParseGroup(src, tokens, tmp, keyValues))
		{
			nextToken = tmp;
			FHL2EntityData entity;
			entity.KeyValues = keyValues;
			ParseCommonKeys(entity);
			out.Add(entity);
			keyValues.Empty();
		}
		else
		{
			const FEntityParserToken& token = tokens[nextToken];
			UE_LOG(LogHL2EntityParser, Error, TEXT("Unexpected token '%s' at %d"), *entityParserTokenTypeNames[(int)token.type], token.start);
			return false;
		}
	}
	return true;
}

inline void FEntityParser::ParseCommonKeys(FHL2EntityData& entity)
{
	const static FName kClassname(TEXT("classname"));
	const static FName kTargetname(TEXT("targetname"));
	const static FName kOrigin(TEXT("origin"));
	const static FName kSpawnflags(TEXT("spawnflags"));
	FString classname;
	if (entity.TryGetString(kClassname, classname)) { entity.Classname = FName(*classname); }
	entity.TryGetString(kTargetname, entity.Targetname);
	entity.TryGetVector(kOrigin, entity.Origin);
	entity.TryGetInt(kSpawnflags, entity.SpawnFlags);
}