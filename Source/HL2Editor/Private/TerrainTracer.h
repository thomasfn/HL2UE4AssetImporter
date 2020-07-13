#pragma once

#include "CoreMinimal.h"
#include "Landscape.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHL2TerrainTracer, Log, All);

class FTerrainTracer
{
private:

	ALandscape* landscape;
	ULandscapeInfo* landscapeInfo;

public:

	FTerrainTracer(ALandscape* landscape);
	
public:

	void Trace();

private:

	bool TryTracePoint(int x, int y, uint16& outHeight);

	bool TryTraceWorld(const FVector& worldPos, float& outHeight);

	static void PatchHole(TArray<uint16>& heightData, const TArray<bool>& heightDataMask, int sizeX, int sizeY, int x, int y);

	static bool FindNearest(const TArray<bool>& heightDataMask, int sizeX, int sizeY, int inX, int inY, int& outX, int& outY);

};
