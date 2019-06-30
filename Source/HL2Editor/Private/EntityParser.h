#pragma once

#include "CoreMinimal.h"
#include "HL2EntityData.h"
#include "ValveKeyValues.h"

class FEntityParser
{
private:

	FEntityParser();
	
public:

	/**
	 * Parses a string from a vbsp entity lump into an array of entities.
	 */
	static bool ParseEntities(const FString& src, TArray<FHL2EntityData>& out);

private:

	static bool ParseGroup(const UValveGroupValue* group, TMap<FName, FString>& outKeyValues, TArray<FEntityLogicOutput>& outLogicOutputs);

	inline static void ParseCommonKeys(FHL2EntityData& entity);
};
