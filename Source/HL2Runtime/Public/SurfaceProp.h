#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "SurfaceProp.generated.h"

UCLASS()
class HL2RUNTIME_API USurfaceProp : public UPhysicalMaterial
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfaceProp)
	float Dampening;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName StepLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName StepRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName BulletImpact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName ScrapeRough;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName ScrapeSmooth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName ImpactHard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropSounds)
	FName ImpactSoft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfaceProp)
	float AudioReflectivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfaceProp)
	float AudioHardnessFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfaceProp)
	float AudioRoughnessFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropMovement)
	float JumpFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropMovement)
	float MaxSpeedFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SurfacePropMovement)
	bool Climbable;
};