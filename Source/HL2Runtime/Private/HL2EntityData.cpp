#include "HL2EntityData.h"

FString FHL2EntityData::GetString(FName key) const
{
	FString tmp;
	if (!TryGetString(key, tmp)) { tmp = FString(); }
	return tmp;
}

bool FHL2EntityData::TryGetString(FName key, FString& out) const
{
	const FString* result = KeyValues.Find(key);
	if (result == nullptr) { return false; }
	out = *result;
	return true;
}

int FHL2EntityData::GetInt(FName key) const
{
	int tmp;
	if (!TryGetInt(key, tmp)) { tmp = 0; }
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
	if (!TryGetBool(key, tmp)) { tmp = false; }
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
	if (!TryGetFloat(key, tmp)) { tmp = 0.0f; }
	return tmp;
}

bool FHL2EntityData::TryGetFloat(FName key, float& out) const
{
	FString value;
	if (!TryGetString(key, value)) { return false; }
	out = FCString::Atof(*value);
	return true;
}

FVector3f FHL2EntityData::GetVector(FName key) const
{
	FVector3f tmp;
	if (!TryGetVector(key, tmp)) { tmp = FVector3f::ZeroVector; }
	return tmp;
}

bool FHL2EntityData::TryGetVector(FName key, FVector3f& out) const
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

FVector4f FHL2EntityData::GetVector4(FName key) const
{
	FVector4f tmp;
	if (!TryGetVector4(key, tmp)) { tmp = FVector4f::Zero(); }
	return tmp;
}

bool FHL2EntityData::TryGetVector4(FName key, FVector4f& out) const
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