#pragma once

#include "UnrealString.h"
#include "ValveBSP/BSPFile.hpp"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2BSPImporter, Log, All);

class FBSPImporter
{
public:

	FBSPImporter();
	virtual ~FBSPImporter();
	
#ifdef WITH_EDITOR

	bool ImportToCurrentLevel(const FString& fileName);

	bool ImportToWorld(const Valve::BSPFile& bspFile, UWorld* world);

	bool ImportBrushesToWorld(const Valve::BSPFile& bspFile, UWorld* world);

#endif

private:

	AActor* ImportBrush(UWorld* world, const Valve::BSPFile& bspFile, uint16 brushIndex);

	void GatherBrushes(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out);

	FPlane ValveToUnrealPlane(const Valve::BSP::cplane_t& plane);

};