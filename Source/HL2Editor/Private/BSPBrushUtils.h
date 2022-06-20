#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"
#include "Materials/MaterialInterface.h"

struct FBSPBrushSide
{
	FPlane4f Plane;
	FVector4f TextureU, TextureV;
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

	static void BuildBrushGeometry(const FBSPBrush& brush, FMeshDescription& meshDesc, FName overrideMaterial = NAME_None, bool alwaysEmitFaces = false);

	static void BuildBrushCollision(const FBSPBrush& brush, UBodySetup* bodySetup);

private:

	static inline void SnapVertex(FVector3f& vertex);

};
