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

};

USTRUCT()
struct FHL2EditorConfig
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FHL2EditorBSPConfig BSP;

};