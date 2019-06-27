#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"
#include "Materials/MaterialInterface.h"

struct FBSPBrushSide
{
	FPlane Plane;
	FVector4 TextureU, TextureV;
	uint16 TextureW, TextureH;
	FName Material;
	uint32 SmoothingGroups;
	bool EmitGeometry;
};

struct FBSPBrush
{
	TArray<FBSPBrushSide> Sides;
	bool CollisionEnabled;
};

class FBSPBrushUtils
{
private:

	FBSPBrushUtils();

public:

	static void BuildBrushGeometry(const FBSPBrush& brush, FMeshDescription& meshDesc);

};


