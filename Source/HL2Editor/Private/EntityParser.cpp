#pragma once

#include "EntityParser.h"

#include "Internationalization/Regex.h"

bool FEntityParser::ParseEntities(const FString& src, TArray<FHL2EntityData>& out)
{
	const UValveDocument* document = UValveDocument::Parse(src);
	if (document == nullptr) { return false; }

	const UValveArrayValue* arrayValue = CastChecked<UValveArrayValue>(document->Root);

	TMap<FName, FString> keyValues;
	TArray<FEntityLogicOutput> logicOutputs;
	for (const UValveValue* value : arrayValue->Items)
	{
		const UValveGroupValue* groupValue = CastChecked<UValveGroupValue>(value);
		if (ParseGroup(groupValue, keyValues, logicOutputs))
		{
			FHL2EntityData entity;
			entity.KeyValues = keyValues;
			entity.LogicOutputs = logicOutputs;
			ParseCommonKeys(entity);
			out.Add(entity);
			keyValues.Empty();
			logicOutputs.Empty();
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool FEntityParser::ParseGroup(const UValveGroupValue* group, TMap<FName, FString>& outKeyValues, TArray<FEntityLogicOutput>& outLogicOutputs)
{
	for (const FValveGroupKeyValue& kv : group->Items)
	{
		const UValvePrimitiveValue* primitiveValue = Cast<UValvePrimitiveValue>(kv.Value);
		if (primitiveValue != nullptr)
		{
			const FString value = primitiveValue->AsString();

			// Check logic output
			TArray<FString> logicArgs;
			value.ParseIntoArray(logicArgs, TEXT(","), false);
			if (logicArgs.Num() >= 4)
			{
				FEntityLogicOutput logicOutput;
				logicArgs[0].TrimEndInline();
				logicArgs[0].TrimStartInline();
				logicOutput.TargetName = FName(*logicArgs[0]);
				logicOutput.OutputName = kv.Key;
				logicArgs[1].TrimEndInline();
				logicArgs[1].TrimStartInline();
				logicOutput.InputName = FName(*logicArgs[1]);
				logicOutput.Delay = FCString::Atof(*logicArgs.Last(1));
				logicOutput.Once = FCString::Atoi(*logicArgs.Last(0)) != -1;
				logicArgs.RemoveAt(logicArgs.Num() - 2, 2);
				logicArgs.RemoveAt(0, 2);
				logicOutput.Params = logicArgs;
				outLogicOutputs.Add(logicOutput);
			}
			else
			{
				outKeyValues.Add(kv.Key, value);
			}
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