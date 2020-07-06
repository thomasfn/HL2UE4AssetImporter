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

};

USTRUCT()
struct FHL2EditorConfig
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FHL2EditorBSPConfig BSP;

};