#pragma once

#include "CoreMinimal.h"

#include "HL2EditorConfig.generated.h"

USTRUCT()
struct FHL2EditorBSPConfig
{
	GENERATED_BODY()

public:

	// Whether to split the map brush geometry into a 2D grid of cells
	UPROPERTY()
	bool UseCells = true;

	// The XY size of each cell
	UPROPERTY()
	int CellSize = 2048;

	// Whether to merge displacements and then split into cells like map brush geometry
	UPROPERTY()
	bool UseDisplacementCells = true;

	// The XY size of each displacement cell
	UPROPERTY()
	int DisplacementCellSize = 2048;

	// Whether to use multiple threads to split cells or not.
	// Should run faster during map import but uses more CPU power and might have strange bugs.
	UPROPERTY()
	bool ParallelizeCellSplitting = true;

	// Whether to use generate lightmap coords for static meshes or not.
	// No longer needed with lumen.
	UPROPERTY()
	bool GenerateLightmapCoords = false;

	// Whether to import env_cubemaps and emit reflection captures for them.
	// No longer needed with lumen.
	UPROPERTY()
	bool EmitReflectionCaptures = false;

	// Whether to prevent dependency on HL2Runtime.
	// If true, a limited set of unreal built-in entities will be used.
	// The map will not function as a HL2 playable map, but can be exported and used in other projects.
	// This is helpful if you're just using the plugin to import map geometry and basic entities without caring about functionality.
	UPROPERTY()
	bool Portable = false;
};

USTRUCT()
struct FHL2EditorMaterialConfig
{
	GENERATED_BODY()

public:

	// Whether to prevent dependency on HL2Runtime.
	// If true, standard unreal materials will be emitted, and shaders will be copied to the local project.
	// Any non-standard metadata, such as surface prop, will be discarded.
	// This is helpful if you're just using the plugin to import textures and materials without caring about functionality.
	UPROPERTY()
	bool Portable = false;
};

USTRUCT()
struct FHL2EditorModelConfig
{
	GENERATED_BODY()

public:

	// Whether to prevent dependency on HL2Runtime.
	// Any non-standard metadata, such as skins or body groups, will be discarded.
	// This is helpful if you're just using the plugin to import static and skeletal meshes without caring about functionality.
	UPROPERTY()
	bool Portable = false;
};

USTRUCT()
struct FHL2EditorConfig
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FHL2EditorBSPConfig BSP;

	UPROPERTY()
	FHL2EditorMaterialConfig Material;

	UPROPERTY()
	FHL2EditorModelConfig Model;

};