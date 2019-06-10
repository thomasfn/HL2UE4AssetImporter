#pragma once;

#include "CoreMinimal.h"

namespace Valve
{
	inline FString ReadString(const void* headerPtr, int offset)
	{
		const char* ptr = ((char*)headerPtr) + offset;
		const auto itemRaw = StringCast<TCHAR, ANSICHAR>(ptr, strlen(ptr) + 1);
		return FString(itemRaw.Get());
	}

	template <class T>
	inline void ReadArray(const void* headerPtr, int offset, int count, TArray<T>& out)
	{
		const T* ptr = (T*)(((uint8*)headerPtr) + offset);
		for (int i = 0; i < count; ++i)
		{
			const T item = *(ptr++);
			out.Add(item);
		}
	}

	template <class T>
	inline void ReadArray(const void* headerPtr, int offset, int count, TArray<const T*>& out)
	{
		const T* ptr = (T*)(((uint8*)headerPtr) + offset);
		for (int i = 0; i < count; ++i)
		{
			out.Add(ptr++);
		}
	}

	template <>
	inline void ReadArray<FString>(const void* headerPtr, int offset, int count, TArray<FString>& out)
	{
		for (int i = 0; i < count; ++i)
		{
			FString str = ReadString(headerPtr, offset);
			out.Add(str);
			offset += str.Len() + 1;
		}
	}
}