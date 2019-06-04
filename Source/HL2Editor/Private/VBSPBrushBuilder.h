#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/BrushBuilder.h"
#include "Materials/MaterialInterface.h"
#include "VBSPBrushBuilder.generated.h"

class ABrush;

USTRUCT()
struct FVBSPBrushPlane
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere)
	FPlane Plane;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;
};

UCLASS(MinimalAPI, autoexpandcategories = BrushSettings, EditInlineNew, meta = (DisplayName = "VBSP"))
class UVBSPBrushBuilder : public UBrushBuilder
{
public:
	GENERATED_BODY()

public:
	UVBSPBrushBuilder(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The planes that bound the brush volume. */
	UPROPERTY(EditAnywhere, Category = BrushSettings)
	TArray<FVBSPBrushPlane> Planes;

	UPROPERTY()
	FName GroupName;

	//~ Begin UBrushBuilder Interface
	virtual void BeginBrush(bool InMergeCoplanars, FName InLayer) override;
	virtual bool EndBrush(UWorld* InWorld, ABrush* InBrush) override;
	virtual int32 GetVertexCount() const override;
	virtual FVector GetVertex(int32 i) const override;
	virtual int32 GetPolyCount() const override;
	virtual bool BadParameters(const FText& msg) override;
	virtual int32 Vertexv(FVector v) override;
	virtual int32 Vertex3f(float X, float Y, float Z) override;
	virtual void Poly3i(int32 Direction, int32 i, int32 j, int32 k, FName ItemName = NAME_None, bool bIsTwoSidedNonSolid = false) override;
	virtual void Poly4i(int32 Direction, int32 i, int32 j, int32 k, int32 l, FName ItemName = NAME_None, bool bIsTwoSidedNonSolid = false) override;
	virtual void PolyBegin(int32 Direction, FName ItemName = NAME_None) override;
	virtual void Polyi(int32 i) override;
	virtual void PolyEnd() override;
	virtual bool Build(UWorld* InWorld, ABrush* InBrush = nullptr) override;
	//~ End UBrushBuilder Interface

	// ~ Begin UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// ~ End UObject interface

private:

	// @todo document
	virtual void BuildVBSP(TArray<FVBSPBrushPlane> planes);

	static bool IntersectLines(const FVector& i1, const FVector& d1, const FVector& i2, const FVector& d2, FVector& out);
};


