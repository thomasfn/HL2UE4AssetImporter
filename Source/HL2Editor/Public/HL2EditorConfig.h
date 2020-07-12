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
	int CellSize = 4096;

	// Whether to merge displacements and then split into cells like map brush geometry
	UPROPERTY()
	bool UseDisplacementCells = true;

	// The XY size of each displacement cell
	UPROPERTY()
	int DisplacementCellSize = 4096;

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