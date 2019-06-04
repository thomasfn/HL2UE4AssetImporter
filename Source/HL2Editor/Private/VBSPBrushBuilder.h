#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/BrushBuilder.h"
#include "Materials/MaterialInterface.h"
#include "VBSPBrushBuilder.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVBSPBrushBuilder, Log, All);

class ABrush;

UCLASS(MinimalAPI, autoexpandcategories = BrushSettings, EditInlineNew, meta = (DisplayName = "VBSP"))
class UVBSPBrushBuilder : public UBrushBuilder
{
public:
	GENERATED_BODY()

public:
	UVBSPBrushBuilder(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** The planes that bound the brush volume. */
	UPROPERTY(EditAnywhere, Category = BrushSettings)
	TArray<FPlane> Planes;

	/** Offset applied to geometry. */
	UPROPERTY(EditAnywhere, Category = BrushSettings)
	FVector Offset;

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

	FVector EvaluateGeometricCenter() const;

private:

	// @todo document
	virtual void BuildVBSP();

	static const float snapThreshold;
	static const float containsThreshold;
	static const float lineParallelThreshold;

	bool CommitPoly(const FPlane& plane, const TArray<FVector>& points);

	bool ContainsPoint(const FVector& point, float threshold = containsThreshold) const;

	static void SnapPoint(FVector& point);

	static FVector SnapPoint(const FVector& point);

	// chainHull_2D(): Andrew's monotone chain 2D convex hull algorithm
	//     Input:  P[] = an array of 2D points 
	//                  presorted by increasing x and y-coordinates
	//     Output: H[] = an array of the convex hull vertices (max is n)
	static void ChainHull2D(const TArray<FVector2D>& P, TArray<FVector2D>& H);
};


