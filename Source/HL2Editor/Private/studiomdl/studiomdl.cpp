#include "studiomdl.h"

void Valve::MDL::studiohdr_t::GetKeyValues(TArray<FString>& keyValues) const
{
	const char* ptr = ((const char*)this) + keyvalue_index;
	for (int i = 0; i < keyvalue_count; ++i)
	{
		int len = strlen(ptr) + 1;
		const auto valueRaw = StringCast<TCHAR, ANSICHAR>(ptr, len);
		FString value(valueRaw.Get());
		keyValues.Add(value);
		ptr += len;
	}
}

bool Valve::MDL::studiohdr_t::HasFlag(studiohdr_flag flag) const
{
	return (flags & (int)flag) != 0;
}