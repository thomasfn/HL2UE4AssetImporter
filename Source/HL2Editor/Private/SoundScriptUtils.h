#pragma once

#include "CoreMinimal.h"
#include "HL2SoundScripts.h"
#include "ValveKeyValues.h"

class FSoundScriptUtils
{
public:
	

	static const TMap<FString, uint8> ChannelNames;

	static const TMap<FString, float> VolumeNames;

	static const TMap<FString, uint8> PitchNames;

	static const TMap<FString, uint8> SoundLevelNames;

	static const TMap<TCHAR, EHL2SoundScriptEntryFlag> CharToFlag;

public:

	static void ParseSoundScriptEntry(const UValveGroupValue* GroupValue, FHL2SoundScriptEntry& OutEntry);

	static void ParseWave(const UValvePrimitiveValue* Value, FString& OutWave, EHL2SoundScriptEntryFlag& OutFlag);

};

template<typename T>
void ParseGenericNumber(UValvePrimitiveValue* Value, const TMap<FString, T>& nameMap, T& outMin, T& outMax, T defaultValue)
{
	if (Value == nullptr)
	{
		outMin = defaultValue;
		outMax = defaultValue;
		return;
	}
	const FString valueText = Value->AsString();
	TArray<FString> segments;
	valueText.ParseIntoArray(segments, TEXT(","), true);
	TArray<T> numbers;
	numbers.Reserve(segments.Num());
	for (FString& segment : segments)
	{
		segment.TrimStartAndEndInline();
		const T* namedValuePtr = nameMap.Find(segment);
		if (namedValuePtr != nullptr)
		{
			numbers.Add(*namedValuePtr);
		}
		else
		{
			numbers.Add((T)FCString::Atof(*segment));
		}
	}
	if (numbers.Num() == 0) { outMin = defaultValue; outMax = defaultValue; }
	else if (numbers.Num() == 1) { outMin = numbers[0]; outMax = numbers[0]; }
	else { outMin = numbers[0]; outMax = numbers[1]; }
}

template<typename T>
void ParseGenericNumber(UValvePrimitiveValue* Value, const TMap<FString, T>& nameMap, T& outValue, T defaultValue)
{
	if (Value == nullptr)
	{
		outValue = defaultValue;
		return;
	}
	FString valueText = Value->AsString();
	valueText.TrimStartAndEndInline();
	const T* namedValuePtr = nameMap.Find(valueText);
	if (namedValuePtr != nullptr)
	{
		outValue = *namedValuePtr;
	}
	else
	{
		outValue = (T)FCString::Atof(*valueText);
	}
}

template<typename T>
void ParseGenericNumber(UValvePrimitiveValue* Value, T& outValue, T defaultValue)
{
	if (Value == nullptr)
	{
		outValue = defaultValue;
		return;
	}
	FString valueText = Value->AsString();
	valueText.TrimStartAndEndInline();
	outValue = (T)FCString::Atof(*valueText);
}
