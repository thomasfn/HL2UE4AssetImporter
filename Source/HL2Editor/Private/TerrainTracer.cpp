#include "TerrainTracer.h"

#include "Misc/ScopedSlowTask.h"

#include "LandscapeEdit.h"
#include "LandscapeDataAccess.h"

DEFINE_LOG_CATEGORY(LogHL2TerrainTracer);

#define LOCTEXT_NAMESPACE ""

FTerrainTracer::FTerrainTracer(ALandscape* landscape)
	: landscape(landscape)
	, landscapeInfo(landscape->GetLandscapeInfo())
{
	check(landscape);
	check(landscapeInfo);
}

void FTerrainTracer::Trace()
{
	UE_LOG(LogHL2TerrainTracer, Log, TEXT("Tracing landscape '%s' against world geometry..."), *landscape->GetActorLabel());

	int minX = MAX_int32;
	int minY = MAX_int32;
	int maxX = -MAX_int32;
	int maxY = -MAX_int32;
	check(landscapeInfo->GetLandscapeExtent(minX, minY, maxX, maxY));
	const int sizeX = (maxX - minX + 1);
	const int sizeY = (maxY - minY + 1);

	TArray<uint16> heightData;
	heightData.AddZeroed(sizeX * sizeY);
	TArray<bool> traceSuccess;
	traceSuccess.AddZeroed(sizeX * sizeY);

	FScopedSlowTask loopProgress(heightData.Num() * 2, LOCTEXT("TerrainTracing", "Tracing landscape..."));
	loopProgress.MakeDialog();
	int successCount = 0;
	for (int y = 0; y < sizeY; ++y)
	{
		for (int x = 0; x < sizeX; ++x)
		{
			loopProgress.EnterProgressFrame(1.0f, LOCTEXT("TerrainTracing_Tracing", "Tracing against world geometry"));
			const bool success = TryTracePoint(x + minX, y + minY, heightData[y * sizeX + x]);
			traceSuccess[y * sizeX + x] = success;
			if (success)
			{
				++successCount;
			}
		}
	}

	UE_LOG(LogHL2TerrainTracer, Log, TEXT("Successfully traced %d of %d points against world geometry"), successCount, sizeX * sizeY);
	UE_LOG(LogHL2TerrainTracer, Log, TEXT("Filling in holes..."));

	for (int y = 0; y < sizeY; ++y)
	{
		for (int x = 0; x < sizeX; ++x)
		{
			loopProgress.EnterProgressFrame(1.0f, LOCTEXT("TerrainTracing_Tracing", "Patching holes"));
			if (!traceSuccess[y * sizeX + x])
			{
				PatchHole(heightData, traceSuccess, sizeX, sizeY, x, y);
			}
		}
	}

	UE_LOG(LogHL2TerrainTracer, Log, TEXT("Committing height data to landscape..."));
	FLandscapeEditDataInterface editData(landscapeInfo);
	editData.SetHeightData(minX, minY, maxX, maxY, heightData.GetData(), 0, true);

	landscapeInfo->PostEditChange();
	landscapeInfo->MarkPackageDirty();

	landscape->PostEditChange();
	landscape->MarkPackageDirty();

	UE_LOG(LogHL2TerrainTracer, Log, TEXT("Done"));
}

bool FTerrainTracer::TryTracePoint(int x, int y, uint16& outHeight)
{
	FVector localPos(x, y, LandscapeDataAccess::GetLocalHeight(MAX_uint16));
	const FTransform actorToWorld = landscape->LandscapeActorToWorld();
	FVector worldPos = actorToWorld.TransformPosition(localPos);
	float worldZ;
	if (!TryTraceWorld(worldPos, worldZ)) { return false; }
	worldPos.Z = worldZ;
	localPos = actorToWorld.InverseTransformPosition(worldPos);
	outHeight = LandscapeDataAccess::GetTexHeight(localPos.Z);
	return true;
}

bool FTerrainTracer::TryTraceWorld(const FVector& worldPos, float& outHeight)
{
	constexpr float MIN_Z = -(1024.0f * 1024.0f); // TODO: Better way to determine a good MIN_Z?
	const FVector endPos(worldPos.X, worldPos.Y, MIN_Z);
	TArray<FHitResult> hitResults;
	const bool didHit = landscape->GetWorld()->LineTraceMultiByChannel(hitResults, worldPos, endPos, ECollisionChannel::ECC_WorldStatic);
	if (!didHit) { return false; }
	for (int i = hitResults.Num() - 1; i >= 0; --i)
	{
		const FHitResult& hitResult = hitResults[i];
		if (IsValid(hitResult.GetActor()) && !hitResult.GetActor()->IsHiddenEd())
		{
			outHeight = hitResult.ImpactPoint.Z;
			return true;
		}
	}
	return false;
}

void FTerrainTracer::PatchHole(TArray<uint16>& heightData, const TArray<bool>& heightDataMask, int sizeX, int sizeY, int x, int y)
{
	int nearestX, nearestY;
	if (!FindNearest(heightDataMask, sizeX, sizeY, x, y, nearestX, nearestY))
	{
		heightData[y * sizeX + x] = LandscapeDataAccess::GetTexHeight(0.0f);
		return;
	}
	heightData[y * sizeX + x] = heightData[nearestY * sizeX + nearestX];
}

bool FTerrainTracer::FindNearest(const TArray<bool>& heightDataMask, int sizeX, int sizeY, int inX, int inY, int& outX, int& outY)
{
	constexpr int MAX_RINGS = 10;
	for (int ring = 1; ring <= MAX_RINGS; ++ring)
	{
		const int ringSize = (ring * 2) - 1;
		const int ringMid = ring - 1;
		const int ringStart = (ringMid - ringSize) + 1;
		for (int i = 0; i < ringSize; ++i)
		{
			const int ringPos = ringStart + i;
			// -y
			{
				const int x = inX + ringPos;
				const int y = inY + ringStart;
				if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && heightDataMask[y * sizeX + x])
				{
					outX = x;
					outY = y;
					return true;
				}
			}
			// +y
			{
				const int x = inX + ringPos;
				const int y = inY - ringStart;
				if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && heightDataMask[y * sizeX + x])
				{
					outX = x;
					outY = y;
					return true;
				}
			}
			// -x
			{
				const int x = inX + ringStart;
				const int y = inY + ringPos;
				if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && heightDataMask[y * sizeX + x])
				{
					outX = x;
					outY = y;
					return true;
				}
			}
			// +x
			{
				const int x = inX - ringStart;
				const int y = inY + ringPos;
				if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && heightDataMask[y * sizeX + x])
				{
					outX = x;
					outY = y;
					return true;
				}
			}
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE