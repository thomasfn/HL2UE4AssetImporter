#include "ValveKeyValues.h"
#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY(LogValveKeyValuesParser);

#pragma region UValveComplexValue

struct CompiledPath
{
	enum Flags
	{
		None,
		ArrayIndexer
	};
	struct Segment
	{
		FName Key;
		int Data;
		Flags Flags;
	};
	TArray<Segment> Segments;
};
static TMap<FName, CompiledPath> CompiledPathMap;
const CompiledPath& GetCompiledPath(const FName path)
{
	CompiledPath* ptr = CompiledPathMap.Find(path);
	if (ptr != nullptr) { return *ptr; }
	CompiledPath& compiledPath = CompiledPathMap.FindOrAdd(path);
	FString pathStr = path.ToString();
	pathStr.TrimStartAndEndInline();
	if (pathStr.IsEmpty()) { return compiledPath; }
	TArray<FString> segments;
	pathStr.ParseIntoArray(segments, TEXT("."), true);
	compiledPath.Segments.Reserve(segments.Num());
	for (int i = 0; i < segments.Num(); ++i)
	{
		FString segmentStr = segments[i];

		// Emit a standard segment
		int arrayIndexerIndex;
		if (!segmentStr.FindChar('[', arrayIndexerIndex))
		{
			arrayIndexerIndex = segmentStr.Len();
		}
		FString beforeArrayIndexer = segmentStr.Left(arrayIndexerIndex);
		beforeArrayIndexer.TrimStartAndEndInline();
		if (beforeArrayIndexer.Len() > 0)
		{
			CompiledPath::Segment segment;
			segment.Key = FName(*beforeArrayIndexer);
			segment.Flags = CompiledPath::None;
			segment.Data = 0;
			compiledPath.Segments.Add(segment);
			segmentStr = segmentStr.RightChop(arrayIndexerIndex);
		}

		// Iterate array indexers
		while (segmentStr.FindChar('[', arrayIndexerIndex))
		{
			// Find end of array indexer
			int arrayIndexerIndexEnd;
			check(segmentStr.FindChar(']', arrayIndexerIndexEnd));
			FString arrayIndexerStr = segmentStr.Mid(arrayIndexerIndex + 1, arrayIndexerIndexEnd - arrayIndexerIndex - 1);

			// Emit an array indexer segment
			{
				CompiledPath::Segment segment;
				segment.Key = FName(*arrayIndexerStr);
				segment.Flags = CompiledPath::ArrayIndexer;
				segment.Data = FCString::Atoi(*arrayIndexerStr);
				compiledPath.Segments.Add(segment);
			}

			segmentStr = segmentStr.RightChop(arrayIndexerIndexEnd + 1);
		}
	}
	return compiledPath;
}

UValveValue* UValveComplexValue::GetValue(FName path) const
{
	const CompiledPath& compiledPath = GetCompiledPath(path);
	UValveValue* curValue = (UValveValue*)this;
	for (int i = 0; i < compiledPath.Segments.Num(); ++i)
	{
		if (curValue == nullptr) { return nullptr; }
		const CompiledPath::Segment& segment = compiledPath.Segments[i];
		if (segment.Flags == CompiledPath::Flags::ArrayIndexer)
		{
			const UValveArrayValue* arrayValue = Cast<UValveArrayValue>(curValue);
			if (arrayValue == nullptr) { return nullptr; }
			if (!arrayValue->Items.IsValidIndex(segment.Data)) { return nullptr; }
			curValue = arrayValue->Items[segment.Data];
		}
		else
		{
			const UValveGroupValue* groupValue = Cast<UValveGroupValue>(curValue);
			if (groupValue == nullptr) { return nullptr; }
			TArray<UValveValue*> items;
			groupValue->GetItems(segment.Key, items);
			if (items.Num() == 0) { return nullptr; }

			// Special case: there could be multiple items with the same key in the group, so peek ahead for an array indexer
			if (i < compiledPath.Segments.Num() - 1 && compiledPath.Segments[i + 1].Flags == CompiledPath::Flags::ArrayIndexer)
			{
				// Use the array indexer to select a key
				const CompiledPath::Segment& arrayIndexerSegment = compiledPath.Segments[++i];
				if (!items.IsValidIndex(arrayIndexerSegment.Data)) { return nullptr; }
				curValue = items[arrayIndexerSegment.Data];
			}
			else
			{
				// Otherwise, just select the first one
				curValue = items[0];
			}
		}
	}
	return curValue;
}

UValvePrimitiveValue* UValveComplexValue::GetPrimitive(FName path) const
{
	UValveValue* value = GetValue(path);
	if (value == nullptr) { return nullptr; }
	return Cast<UValvePrimitiveValue>(value);
}

UValveGroupValue* UValveComplexValue::GetGroup(FName path) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	return Cast<UValveGroupValue>(rawValue);
}

UValveArrayValue* UValveComplexValue::GetArray(FName path) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	return Cast<UValveArrayValue>(rawValue);
}

bool UValveComplexValue::GetInt(FName path, int& outValue) const
{
	UValvePrimitiveValue* primValue = GetPrimitive(path);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsInt();
	return true;
}

bool UValveComplexValue::GetFloat(FName path, float& outValue) const
{
	UValvePrimitiveValue* primValue = GetPrimitive(path);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsFloat();
	return true;
}

bool UValveComplexValue::GetBool(FName path, bool& outValue) const
{
	UValvePrimitiveValue* primValue = GetPrimitive(path);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsBool();
	return true;
}

bool UValveComplexValue::GetString(FName path, FString& outValue) const
{
	UValvePrimitiveValue* primValue = GetPrimitive(path);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsString();
	return true;
}

bool UValveComplexValue::GetName(FName path, FName& outValue) const
{
	UValvePrimitiveValue* primValue = GetPrimitive(path);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsName();
	return true;
}

#pragma endregion

#pragma region UValveGroupValue

UValveValue* UValveGroupValue::GetItem(FName key) const
{
	for (const FValveGroupKeyValue& kv : Items)
	{
		if (kv.Key == key)
		{
			return kv.Value;
		}
	}
	return nullptr;
}

int UValveGroupValue::GetItems(FName key, TArray<UValveValue*>& outItems) const
{
	int num = 0;
	for (const FValveGroupKeyValue& kv : Items)
	{
		if (kv.Key == key)
		{
			outItems.Add(kv.Value);
			++num;
		}
	}
	return num;
}

#pragma endregion

#pragma region UValvePrimitiveValue

float UValvePrimitiveValue::AsFloat(float defaultValue) const
{
	return FCString::Atof(*Value); // todo: handle unparseable
}

int UValvePrimitiveValue::AsInt(int defaultValue) const
{
	return FCString::Atoi(*Value); // todo: handle unparseable
}

bool UValvePrimitiveValue::AsBool(bool defaultValue) const
{
	return Value.ToBool(); // todo: handle unparseable
}

FString UValvePrimitiveValue::AsString() const
{
	return Value;
}

FName UValvePrimitiveValue::AsName() const
{
	return FName(*Value);
}

#pragma endregion

#pragma region UValveDocument

enum class TokenType
{
	Whitespace,
	SingleLineComment,
	Newline,
	OpenGroup,
	CloseGroup,
	QuotedString,
	UnquotedString,
	TokenType_Count
};

struct Token
{
	TokenType type;
	int start;
	int end;
};

const FString tokenTypeNames[] =
{
	TEXT("Whitespace"),
	TEXT("SingleLineComment"),
	TEXT("Newline"),
	TEXT("OpenGroup"),
	TEXT("CloseGroup"),
	TEXT("QuotedString"),
	TEXT("UnquotedString")
};

constexpr int numTokenTypeNames = sizeof(tokenTypeNames) / sizeof(FString);
static_assert(numTokenTypeNames == (int)TokenType::TokenType_Count, "ValueKeyValues.cpp: Each token type must have a name");

bool Tokenise(const FString& src, TArray<Token>& out)
{
	const static FRegexPattern patterns[] =
	{
		FRegexPattern(TEXT("[ \\t\\r]+")), // Whitespace
		FRegexPattern(TEXT("//[^\\n]*")), // SingleLineComment
		FRegexPattern(TEXT("\\n+")), // Newline
		FRegexPattern(TEXT("\\{")), // OpenGroup
		FRegexPattern(TEXT("\\}")), // CloseGroup
		FRegexPattern(TEXT("\"((?:\\\\\"|[^\"\n])*)\"")), // QuotedString
		FRegexPattern(TEXT("((?:\\\\\"|[^\"\\s\\{\\}])*)")) // UnquotedString
	};
	constexpr int numPatterns = sizeof(patterns) / sizeof(FRegexPattern);
	static_assert(numPatterns == (int)TokenType::TokenType_Count, "ValueKeyValues.cpp: Each token type must have a regex pattern");
	int curPos = 0;
	while (curPos < src.Len())
	{
		bool matched = false;
		for (int i = 0; i < numPatterns; ++i)
		{
			FRegexMatcher matcher(patterns[i], src);
			matcher.SetLimits(curPos, src.Len());
			if (matcher.FindNext() && matcher.GetMatchBeginning() == curPos && matcher.GetMatchEnding() > curPos)
			{
				matched = true;
				if (i > 1) // ignore whitespace and comments
				{
					Token token;
					token.type = (TokenType)i;
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
			UE_LOG(LogValveKeyValuesParser, Error, TEXT("Unexpected '%c' at %d"), src[curPos], curPos);
			return false;
		}
	}
	return true;
}

void ReadToken(const FString& src, const Token& token, FString& out)
{
	out = FString(token.end - token.start, *src + token.start);
}

UValveValue* ParseValue(const FString& src, const TArray<Token>& tokens, int& nextToken, UObject* outer);
UValveComplexValue* ParseGroup(const FString& src, const TArray<Token>& tokens, int& nextToken, UObject* outer);

// Parses any kind of value from the token stream
UValveValue* ParseValue(const FString& src, const TArray<Token>& tokens, int& nextToken, UObject* outer)
{
	// Allow some newlines
	while (tokens[nextToken].type == TokenType::Newline) { ++nextToken; }

	switch (tokens[nextToken].type) // Peek
	{
		case TokenType::QuotedString:
		{
			UValvePrimitiveValue* stringValue = NewObject<UValvePrimitiveValue>(outer);
			FString tmp;
			ReadToken(src, tokens[nextToken++], tmp); // Consume
			tmp.RemoveFromStart(TEXT("\""));
			tmp.RemoveFromEnd(TEXT("\""));
			stringValue->Value = tmp;
			return stringValue;
		}
		case TokenType::UnquotedString:
		{
			// Special case: multiple unquoted strings in sequence should be considered as one and concatenated
			TArray<FString> segments;
			while (tokens[nextToken].type == TokenType::UnquotedString)
			{
				FString segment;
				ReadToken(src, tokens[nextToken++], segment); // Consume
				segments.Add(segment);
			}
			UValvePrimitiveValue* stringValue = NewObject<UValvePrimitiveValue>(outer);
			stringValue->Value = FString::Join(segments, TEXT(" "));
			stringValue->Value.TrimStartAndEndInline();
			return stringValue;
		}
		case TokenType::OpenGroup:
		{
			return ParseGroup(src, tokens, nextToken, outer);
		}
		default:
		{
			UE_LOG(LogValveKeyValuesParser, Error, TEXT("Expecting value, got '%s' at %d"), *tokenTypeNames[(int)tokens[nextToken].type], tokens[nextToken].start);
			return nullptr;
		}
	}
}

// Parses a group value from the token stream
UValveComplexValue* ParseGroup(const FString& src, const TArray<Token>& tokens, int& nextToken, UObject* outer)
{
	// OpenGroup
	if (tokens[nextToken++].type != TokenType::OpenGroup) { return nullptr; }

	UValveGroupValue* groupValue = nullptr;
	UValveArrayValue* arrayValue = nullptr;

	// String | CloseGroup
	while (tokens.IsValidIndex(nextToken) && tokens[nextToken].type != TokenType::CloseGroup)
	{
		const Token& peekToken = tokens[nextToken]; // Peek
		if (peekToken.type == TokenType::Newline)
		{
			++nextToken; // Consume
		}
		else if (peekToken.type == TokenType::QuotedString || peekToken.type == TokenType::UnquotedString)
		{
			if (arrayValue != nullptr)
			{
				arrayValue->MarkPendingKill();
				UE_LOG(LogValveKeyValuesParser, Error, TEXT("Expecting value for array-group, got key at %d"), peekToken.start);
				return nullptr;
			}
			if (groupValue == nullptr)
			{
				groupValue = NewObject<UValveGroupValue>(outer);
			}

			// Consume
			++nextToken;

			// Key
			FString keyStr;
			ReadToken(src, peekToken, keyStr);
			if (peekToken.type == TokenType::QuotedString)
			{
				keyStr.RemoveFromStart(TEXT("\""));
				keyStr.RemoveFromEnd(TEXT("\""));
			}

			// Value
			UValveValue* value = ParseValue(src, tokens, nextToken, groupValue);
			if (value == nullptr)
			{
				groupValue->MarkPendingKill();
				return nullptr;
			}

			// Insert
			FValveGroupKeyValue keyValue;
			keyValue.Key = FName(*keyStr);
			keyValue.Value = value;
			groupValue->Items.Add(keyValue);
		}
		else if (peekToken.type == TokenType::OpenGroup)
		{
			if (groupValue != nullptr)
			{
				groupValue->MarkPendingKill();
				UE_LOG(LogValveKeyValuesParser, Error, TEXT("Expecting key for keyvalue-group, got group at %d"), peekToken.start);
				return nullptr;
			}
			if (arrayValue == nullptr)
			{
				arrayValue = NewObject<UValveArrayValue>(outer);
			}

			// Value
			UValveValue* value = ParseValue(src, tokens, nextToken, arrayValue);
			if (value == nullptr)
			{
				arrayValue->MarkPendingKill();
				return nullptr;
			}

			// Insert
			arrayValue->Items.Add(value);
		}
		else
		{
			UE_LOG(LogValveKeyValuesParser, Error, TEXT("Expecting key-value or value, got '%s' at %d"), *tokenTypeNames[(int)peekToken.type], peekToken.start);
			if (groupValue != nullptr) { groupValue->MarkPendingKill(); }
			if (arrayValue != nullptr) { arrayValue->MarkPendingKill(); }
			return nullptr;
		}
	}

	// CloseGroup
	nextToken++;

	if (groupValue != nullptr) { return groupValue; }
	if (arrayValue != nullptr) { return arrayValue; }

	// If we got this far, the group is empty (e.g. "{}") so create and return an empty group
	return NewObject<UValveGroupValue>(outer);
}

UValveDocument* UValveDocument::Parse(const FString& text, UObject* outer)
{
	TArray<Token> tokens;
	{ Token token; token.start = 0; token.end = 0; token.type = TokenType::OpenGroup; tokens.Add(token); }
	if (!Tokenise(text, tokens)) { return nullptr; }
	{ Token token; token.start = 0; token.end = 0; token.type = TokenType::CloseGroup; tokens.Add(token); }

	if (outer == nullptr) { outer = (UObject*)GetTransientPackage(); }

	int pos = 0;
	UValveComplexValue* value = ParseGroup(text, tokens, pos, outer);
	if (value == nullptr) { return nullptr; }

	UValveDocument* doc = NewObject<UValveDocument>(outer);
	doc->Root = value;
	return doc;
}

UValveValue* UValveDocument::GetValue(FName path) const
{
	return Root->GetValue(path);
}

bool UValveDocument::GetInt(FName path, int& outValue) const
{
	return Root->GetInt(path, outValue);
}

bool UValveDocument::GetFloat(FName path, float& outValue) const
{
	return Root->GetFloat(path, outValue);
}

bool UValveDocument::GetBool(FName path, bool& outValue) const
{
	return Root->GetBool(path, outValue);
}

bool UValveDocument::GetString(FName path, FString& outValue) const
{
	return Root->GetString(path, outValue);
}

bool UValveDocument::GetName(FName path, FName& outValue) const
{
	return Root->GetName(path, outValue);
}

UValveGroupValue* UValveDocument::GetGroup(FName path) const
{
	return Root->GetGroup(path);
}

UValveArrayValue* UValveDocument::GetArray(FName path) const
{
	return Root->GetArray(path);
}

#pragma endregion