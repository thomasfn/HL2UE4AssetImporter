#pragma once

#include "CoreMinimal.h"

#include "HL2EditorConfig.generated.h"

USTRUCT()
struct FHL2EditorCellsConfig
{
	GENERATED_BODY()

public:

	// Whether to split the geometry into cells
	UPROPERTY()
	bool UseCells = true;

	// The XY size of each cell
	UPROPERTY()
	int CellSize = 2048;

	// Whether to use multiple threads to split cells or not.
	UPROPERTY()
	bool ParallelizeCellSplitting = true;
};

USTRUCT()
struct FHL2EditorBSPConfig
{
	GENERATED_BODY()

public:

	// Whether to split the map brush geometry into cells
	UPROPERTY()
	FHL2EditorCellsConfig BrushGeometryCells;

	// Whether to split displacement geometry into cells
	UPROPERTY()
	FHL2EditorCellsConfig DisplacementCells;

	// Whether to split detail prop geometry into cells
	UPROPERTY()
	FHL2EditorCellsConfig DetailPropCells;

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