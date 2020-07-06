#pragma once

#include "CoreMinimal.h"

#include "HL2EntityData.h"
#include "BaseEntity.h"
#include "ValveBSP/BSPFile.hpp"

using PortableEntityImporterFunc = TFunction<AActor*(const FHL2EntityData&)>;

class FEntityEmitter
{
private:

	UWorld* world;
	const Valve::BSPFile& bspFile;
	const TArray<UStaticMesh*>& bspModels;
	AVBSPInfo* vbspInfo;

	TMap<FName, PortableEntityImporterFunc> portableEntityImporters;

public:

	FEntityEmitter(UWorld* world, const Valve::BSPFile& bspFile, const TArray<UStaticMesh*>& bspModels, AVBSPInfo* vbspInfo);

public:

	void GenerateActors(const TArrayView<FHL2EntityData>& entityDatas, FScopedSlowTask* progress = nullptr);

private:

	ABaseEntity* ImportEntityToWorld(const FHL2EntityData& entityData);

	AActor* ImportPortableEntityToWorld(const FHL2EntityData& entityData);
};
