#include "HL2RuntimePrivatePCH.h"

#include "ValveKeyValues.h"

#pragma region UValveGroupValue

UValveValue* UValveGroupValue::GetItem(FName key)
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

int UValveGroupValue::GetItems(FName key, TArray<UValveValue*>& outItems)
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
	return defaultValue;
}

int UValvePrimitiveValue::AsInt(int defaultValue) const
{
	return defaultValue;
}

bool UValvePrimitiveValue::AsBool(bool defaultValue) const
{
	return defaultValue;
}

FString UValvePrimitiveValue::AsString() const
{
	return TEXT("");
}

FName UValvePrimitiveValue::AsName() const
{
	return FName(*AsString());
}

#pragma endregion

#pragma region UValveIntegerValue

float UValveIntegerValue::AsFloat(float defaultValue) const
{
	return Value;
}

int UValveIntegerValue::AsInt(int defaultValue) const
{
	return Value;
}

bool UValveIntegerValue::AsBool(bool defaultValue) const
{
	return Value > 0;
}

FString UValveIntegerValue::AsString() const
{
	return FString::Printf(TEXT("%d"), Value);
}

#pragma endregion

#pragma region UValveFloatValue

float UValveFloatValue::AsFloat(float defaultValue) const
{
	return Value;
}

int UValveFloatValue::AsInt(int defaultValue) const
{
	return (int)Value;
}

bool UValveFloatValue::AsBool(bool defaultValue) const
{
	return Value > 0.0f;
}

FString UValveFloatValue::AsString() const
{
	return FString::Printf(TEXT("%f"), Value);
}

#pragma endregion

#pragma region UValveStringValue

float UValveStringValue::AsFloat(float defaultValue) const
{
	return FCString::Atof(*Value); // todo: handle unparseable
}

int UValveStringValue::AsInt(int defaultValue) const
{
	return FCString::Atoi(*Value); // todo: handle unparseable
}

bool UValveStringValue::AsBool(bool defaultValue) const
{
	return Value.ToBool(); // todo: handle unparseable
}

FString UValveStringValue::AsString() const
{
	return Value;
}

#pragma endregion

#pragma region UValveDocument

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
		const FString& segmentStr = segments[i];

		// See if it contains an array indexer
		int arrayIndexerIndex;
		if (segmentStr.FindChar('[', arrayIndexerIndex))
		{
			// Emit a standard segment
			FString beforeArrayIndexer = segmentStr.Left(arrayIndexerIndex);
			beforeArrayIndexer.TrimStartAndEndInline();
			if (beforeArrayIndexer.Len() > 0)
			{
				CompiledPath::Segment segment;
				segment.Key = FName(*beforeArrayIndexer);
				segment.Flags = CompiledPath::None;
				segment.Data = 0;
				compiledPath.Segments.Add(segment);
			}

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

			check(arrayIndexerIndexEnd == segmentStr.Len() - 1);
		}
		else
		{
			CompiledPath::Segment segment;
			segment.Key = FName(*segmentStr);
			segment.Flags = CompiledPath::None;
			segment.Data = 0;
			compiledPath.Segments.Add(segment);
		}
	}
	return compiledPath;
}

UValveDocument* UValveDocument::Parse(const FString& text, UObject* outer)
{
	return nullptr;
}

UValveValue* UValveDocument::GetValue(FName path) const
{
	const CompiledPath& compiledPath = GetCompiledPath(path);
	UValveValue* curValue = Root;
	for (int i = 0; i < compiledPath.Segments.Num(); ++i)
	{
		if (curValue == nullptr) { return nullptr; }
		const CompiledPath::Segment& segment = compiledPath.Segments[i];
		if (segment.Flags == CompiledPath::Flags::ArrayIndexer)
		{
			UValveArrayValue* arrayValue = Cast<UValveArrayValue>(curValue);
			if (arrayValue == nullptr) { return nullptr; }
			if (!arrayValue->Items.IsValidIndex(segment.Data)) { return nullptr; }
			curValue = arrayValue->Items[segment.Data];
		}
		else
		{
			UValveGroupValue* groupValue = Cast<UValveGroupValue>(curValue);
			if (groupValue == nullptr) { return nullptr; }
			TArray<UValveValue*> items;
			groupValue->GetItems(segment.Key, items);
			if (items.Num() == 0) { return nullptr; }

			// Special case: there could be multiple items with the same key in the group, so peek ahead for an array indexer
			if (i < compiledPath.Segments.Num() - 1 && compiledPath.Segments[i + 1].Flags == CompiledPath::Flags::ArrayIndexer)
			{
				// Use the array indexer to select a key
				const CompiledPath::Segment& arrayIndexerSegment = compiledPath.Segments[i++];
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

bool UValveDocument::GetInt(FName path, int& outValue) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(rawValue);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsInt();
	return true;
}

bool UValveDocument::GetFloat(FName path, float& outValue) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(rawValue);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsFloat();
	return true;
}

bool UValveDocument::GetBool(FName path, bool& outValue) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(rawValue);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsBool();
	return true;
}

bool UValveDocument::GetString(FName path, FString& outValue) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(rawValue);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsString();
	return true;
}

bool UValveDocument::GetName(FName path, FName& outValue) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(rawValue);
	if (primValue == nullptr) { return false; }
	outValue = primValue->AsName();
	return true;
}

UValveGroupValue* UValveDocument::GetGroup(FName path) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	return Cast<UValveGroupValue>(rawValue);
}

UValveArrayValue* UValveDocument::GetArray(FName path) const
{
	UValveValue* rawValue = GetValue(path);
	if (rawValue == nullptr) { return false; }
	return Cast<UValveArrayValue>(rawValue);
}

#pragma endregion