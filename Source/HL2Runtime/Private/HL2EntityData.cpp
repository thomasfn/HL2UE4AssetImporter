#include "HL2RuntimePrivatePCH.h"

#include "HL2EntityData.h"

FString FHL2EntityData::GetString(FName key) const
{
	FString tmp;
	TryGetString(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetString(FName key, FString& out) const
{
	if (!KeyValues.Contains(key)) { return false; }
	out = KeyValues[key];
	return true;
}

int FHL2EntityData::GetInt(FName key) const
{
	int tmp;
	TryGetInt(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetInt(FName key, int& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	out = FCString::Atoi(*value);
	return true;
}

bool FHL2EntityData::GetBool(FName key) const
{
	bool tmp;
	TryGetBool(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetBool(FName key, bool& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	out = value.ToBool();
	return true;
}

float FHL2EntityData::GetFloat(FName key) const
{
	float tmp;
	TryGetFloat(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetFloat(FName key, float& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	out = FCString::Atof(*value);
	return true;
}

FVector FHL2EntityData::GetVector(FName key) const
{
	FVector tmp;
	TryGetVector(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetVector(FName key, FVector& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	TArray<FString> segments;
	if (value.ParseIntoArray(segments, TEXT(" "), true) < 3) { return false; }
	out.X = FCString::Atof(*segments[0]);
	out.Y = FCString::Atof(*segments[1]);
	out.Z = FCString::Atof(*segments[2]);
	return true;
}

FVector4 FHL2EntityData::GetVector4(FName key) const
{
	FVector4 tmp;
	TryGetVector4(key, tmp);
	return tmp;
}

bool FHL2EntityData::TryGetVector4(FName key, FVector4& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	TArray<FString> segments;
	if (value.ParseIntoArray(segments, TEXT(" "), true) < 3) { return false; }
	out.X = FCString::Atof(*segments[0]);
	out.Y = FCString::Atof(*segments[1]);
	out.Z = FCString::Atof(*segments[2]);
	out.W = FCString::Atof(*segments[3]);
	return true;
}