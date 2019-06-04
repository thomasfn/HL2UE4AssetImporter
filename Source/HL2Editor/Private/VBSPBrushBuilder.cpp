#pragma once

#include "VBSPBrushBuilder.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Engine/Selection.h"
#include "Engine/Polys.h"
#include "SnappingUtils.h"
//#include "BSPOps.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "BrushBuilder"

UVBSPBrushBuilder::UVBSPBrushBuilder(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName NAME_VBSP;
		FConstructorStatics()
			: NAME_VBSP(TEXT("VBSP"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	
	GroupName = ConstructorStatics.NAME_VBSP;
	BitmapFilename = TEXT("BBGeneric");
	ToolTip = TEXT("BrushBuilderName_Generic");
	NotifyBadParams = true;
}

void UVBSPBrushBuilder::BeginBrush(bool InMergeCoplanars, FName InLayer)
{
	Layer = InLayer;
	MergeCoplanars = InMergeCoplanars;
	Vertices.Empty();
	Polys.Empty();
}

/**
 * Validate a brush, and set iLinks on all EdPolys to index of the
 * first identical EdPoly in the list, or its index if it's the first.
 * Not transactional.
 */
void bspValidateBrush(UModel* Brush, bool ForceValidate, bool DoStatusUpdate)
{
	check(Brush != nullptr);
	Brush->Modify();
	if (ForceValidate || !Brush->Linked)
	{
		Brush->Linked = 1;
		for (int32 i = 0; i < Brush->Polys->Element.Num(); i++)
		{
			Brush->Polys->Element[i].iLink = i;
		}
		int32 n = 0;
		for (int32 i = 0; i < Brush->Polys->Element.Num(); i++)
		{
			FPoly* EdPoly = &Brush->Polys->Element[i];
			if (EdPoly->iLink == i)
			{
				for (int32 j = i + 1; j < Brush->Polys->Element.Num(); j++)
				{
					FPoly* OtherPoly = &Brush->Polys->Element[j];
					if
						(OtherPoly->iLink == j
							&& OtherPoly->Material == EdPoly->Material
							&& OtherPoly->TextureU == EdPoly->TextureU
							&& OtherPoly->TextureV == EdPoly->TextureV
							&& OtherPoly->PolyFlags == EdPoly->PolyFlags
							&& (OtherPoly->Normal | EdPoly->Normal) > 0.9999)
					{
						float Dist = FVector::PointPlaneDist(OtherPoly->Vertices[0], EdPoly->Vertices[0], EdPoly->Normal);
						if (Dist > -0.001 && Dist < 0.001)
						{
							OtherPoly->iLink = i;
							n++;
						}
					}
				}
			}
		}
		// 		UE_LOG(LogBSPOps, Log,  TEXT("BspValidateBrush linked %i of %i polys"), n, Brush->Polys->Element.Num() );
	}

	// Build bounds.
	Brush->BuildBound();
}

bool UVBSPBrushBuilder::EndBrush(UWorld* InWorld, ABrush* InBrush)
{
	//!!validate
	check(InWorld != nullptr);
	ABrush* BuilderBrush = (InBrush != nullptr) ? InBrush : InWorld->GetDefaultBrush();

	// Ensure the builder brush is unhidden.
	BuilderBrush->bHidden = false;
	BuilderBrush->bHiddenEdLayer = false;

	AActor* Actor = GEditor->GetSelectedActors()->GetTop<AActor>();
	FVector Location;
	if (InBrush == nullptr)
	{
		Location = Actor ? Actor->GetActorLocation() : BuilderBrush->GetActorLocation();
	}
	else
	{
		Location = InBrush->GetActorLocation();
	}

	UModel* Brush = BuilderBrush->Brush;
	if (Brush == nullptr)
	{
		return true;
	}

	Brush->Modify();
	BuilderBrush->Modify();

	FRotator Temp(0.0f, 0.0f, 0.0f);
	FSnappingUtils::SnapToBSPVertex(Location, FVector::ZeroVector, Temp);
	BuilderBrush->SetActorLocation(Location, false);
	BuilderBrush->SetPivotOffset(FVector::ZeroVector);

	// Try and maintain the materials assigned to the surfaces. 
	TArray<FPoly> CachedPolys;
	UMaterialInterface* CachedMaterial = nullptr;
	if (Brush->Polys->Element.Num() == Polys.Num())
	{
		// If the number of polygons match we assume its the same shape.
		CachedPolys.Append(Brush->Polys->Element);
	}
	else if (Brush->Polys->Element.Num() > 0)
	{
		// If the polygons have changed check if we only had one material before. 
		CachedMaterial = Brush->Polys->Element[0].Material;
		if (CachedMaterial != NULL)
		{
			for (auto Poly : Brush->Polys->Element)
			{
				if (CachedMaterial != Poly.Material)
				{
					CachedMaterial = NULL;
					break;
				}
			}
		}
	}

	// Clear existing polys.
	Brush->Polys->Element.Empty();

	const bool bUseCachedPolysMaterial = CachedPolys.Num() > 0;
	int32 CachedPolyIdx = 0;
	for (TArray<FBuilderPoly>::TIterator It(Polys); It; ++It)
	{
		if (It->Direction < 0)
		{
			for (int32 i = 0; i < It->VertexIndices.Num() / 2; i++)
			{
				Exchange(It->VertexIndices[i], It->VertexIndices.Last(i));
			}
		}

		FPoly Poly;
		Poly.Init();
		Poly.ItemName = It->ItemName;
		Poly.Base = Vertices[It->VertexIndices[0]];
		Poly.PolyFlags = It->PolyFlags;

		// Try and maintain the polygons material where possible
		Poly.Material = (bUseCachedPolysMaterial) ? CachedPolys[CachedPolyIdx++].Material : CachedMaterial;

		for (int32 j = 0; j < It->VertexIndices.Num(); j++)
		{
			new(Poly.Vertices) FVector(Vertices[It->VertexIndices[j]]);
		}
		if (Poly.Finalize(BuilderBrush, 1) == 0)
		{
			new(Brush->Polys->Element)FPoly(Poly);
		}
	}

	if (MergeCoplanars)
	{
		GEditor->bspMergeCoplanars(Brush, 0, 1);
		bspValidateBrush(Brush, 1, 1);
	}
	Brush->Linked = 1;
	bspValidateBrush(Brush, 0, 1);
	Brush->BuildBound();

	GEditor->RedrawLevelEditingViewports();
	GEditor->SetPivot(BuilderBrush->GetActorLocation(), false, true);

	BuilderBrush->ReregisterAllComponents();

	return true;
}

int32 UVBSPBrushBuilder::GetVertexCount() const
{
	return Vertices.Num();
}

FVector UVBSPBrushBuilder::GetVertex(int32 i) const
{
	return Vertices.IsValidIndex(i) ? Vertices[i] : FVector::ZeroVector;
}

int32 UVBSPBrushBuilder::GetPolyCount() const
{
	return Polys.Num();
}

bool UVBSPBrushBuilder::BadParameters(const FText& Msg)
{
	if (NotifyBadParams)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Msg"), Msg);
		FNotificationInfo Info(FText::Format(LOCTEXT("BadParameters", "Bad parameters in brush builder\n{Msg}"), Arguments));
		Info.bFireAndForget = true;
		Info.ExpireDuration = Msg.IsEmpty() ? 4.0f : 6.0f;
		Info.bUseLargeFont = Msg.IsEmpty();
		Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	return 0;
}

int32 UVBSPBrushBuilder::Vertexv(FVector V)
{
	int32 Result = Vertices.Num();
	new(Vertices)FVector(V);

	return Result;
}

int32 UVBSPBrushBuilder::Vertex3f(float X, float Y, float Z)
{
	int32 Result = Vertices.Num();
	new(Vertices)FVector(X, Y, Z);
	return Result;
}

void UVBSPBrushBuilder::Poly3i(int32 Direction, int32 i, int32 j, int32 k, FName ItemName, bool bIsTwoSidedNonSolid)
{
	new(Polys)FBuilderPoly;
	Polys.Last().Direction = Direction;
	Polys.Last().ItemName = ItemName;
	new(Polys.Last().VertexIndices)int32(i);
	new(Polys.Last().VertexIndices)int32(j);
	new(Polys.Last().VertexIndices)int32(k);
	Polys.Last().PolyFlags = PF_DefaultFlags | (bIsTwoSidedNonSolid ? (PF_TwoSided | PF_NotSolid) : 0);
}

void UVBSPBrushBuilder::Poly4i(int32 Direction, int32 i, int32 j, int32 k, int32 l, FName ItemName, bool bIsTwoSidedNonSolid)
{
	new(Polys)FBuilderPoly;
	Polys.Last().Direction = Direction;
	Polys.Last().ItemName = ItemName;
	new(Polys.Last().VertexIndices)int32(i);
	new(Polys.Last().VertexIndices)int32(j);
	new(Polys.Last().VertexIndices)int32(k);
	new(Polys.Last().VertexIndices)int32(l);
	Polys.Last().PolyFlags = PF_DefaultFlags | (bIsTwoSidedNonSolid ? (PF_TwoSided | PF_NotSolid) : 0);
}

void UVBSPBrushBuilder::PolyBegin(int32 Direction, FName ItemName)
{
	new(Polys)FBuilderPoly;
	Polys.Last().ItemName = ItemName;
	Polys.Last().Direction = Direction;
	Polys.Last().PolyFlags = PF_DefaultFlags;
}

void UVBSPBrushBuilder::Polyi(int32 i)
{
	new(Polys.Last().VertexIndices)int32(i);
}

void UVBSPBrushBuilder::PolyEnd()
{
}

void UVBSPBrushBuilder::BuildVBSP(TArray<FVBSPBrushPlane> planes)
{
	const int planeCnt = planes.Num();

	// Iterate all planes
	for (int i = 0; i < planeCnt; ++i)
	{
		const FVBSPBrushPlane& plane = planes[i];

		TArray<TTuple<FVector, FVector>> lines;
		TArray<FVector> points;

		// Iterate all other planes
		for (int j = 0; j < planeCnt; ++j)
		{
			if (j != i)
			{
				// Intersect both planes, if it produces a line then emit that
				const FVBSPBrushPlane& otherPlane = planes[j];
				FVector vI, vD;
				if (FMath::IntersectPlanes2(vI, vD, plane.Plane, otherPlane.Plane))
				{
					lines.Add(TTuple<FVector, FVector>(vI, vD));
				}
			}
		}

		// Iterate all lines
		for (const auto& line : lines)
		{
			// Iterate all other lines
			for (const auto& otherLine : lines)
			{
				// Intersect both lines, if it produces a point then emit that but only if the point is behind ALL planes
				FVector point;
				if (IntersectLines(line.Key, line.Value, otherLine.Key, otherLine.Value, point))
				{
					FRotator Temp(0.0f, 0.0f, 0.0f);
					FSnappingUtils::SnapToBSPVertex(point, FVector::ZeroVector, Temp);
					bool isOK = true;
					/*for (const FVBSPBrushPlane& otherPlane : planes)
					{
						if (otherPlane.Plane.PlaneDot(point) > 0.0f)
						{
							isOK = false;
							break;
						}
					}*/
					if (isOK)
					{
						points.AddUnique(point);
					}
				}
			}
		}

		// TODO: Convex hull around the points
	}
}

bool UVBSPBrushBuilder::IntersectLines(const FVector& i1, const FVector& d1, const FVector& i2, const FVector& d2, FVector& out)
{
	const FVector& fromA = i1;
	const FVector& fromB = i2;
	const FVector toA = i1 + d1;
	const FVector toB = i2 + d2;

	const FVector da = fromB - fromA;
	const FVector db = toB - toA;
	const FVector dc = toA - fromA;

	const FVector crossDaDb = FVector::CrossProduct(da, db);
	float prod = crossDaDb.X * crossDaDb.X + crossDaDb.Y * crossDaDb.Y + crossDaDb.Z * crossDaDb.Z;
	if (prod == 0 || FVector::DotProduct(dc, crossDaDb) != 0)
	{
		return false;
	}
	const float res = FVector::DotProduct(FVector::CrossProduct(dc, db), FVector::CrossProduct(da, db)) / prod;

	out = fromA + da * FVector(res, res, res);

	FVector fromAToIntersectPoint = out - fromA;
	FVector fromBToIntersectPoint = out - fromB;
	FVector toAToIntersectPoint = out - toA;
	FVector toBToIntersectPoint = out - toB;
	if (FVector::DotProduct(fromAToIntersectPoint, fromBToIntersectPoint) <= 0 && FVector::DotProduct(toAToIntersectPoint, toBToIntersectPoint) <= 0)
	{
		return true;
	}
	return false;
}

bool UVBSPBrushBuilder::Build(UWorld* InWorld, ABrush* InBrush)
{
	if (Planes.Num() < 4)
		return BadParameters(LOCTEXT("VBSPNotEnoughPlanes", "Not enough planes in VBSP brush"));
	
	// TODO: Extra validation on planes? Check for unbound volume etc

	BeginBrush(false, GroupName);
	BuildVBSP(Planes);
	return EndBrush(InWorld, InBrush);
}

void UVBSPBrushBuilder::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!GIsTransacting)
	{
		// Rebuild brush on property change
		ABrush* Brush = Cast<ABrush>(GetOuter());
		UWorld* BrushWorld = Brush ? Brush->GetWorld() : nullptr;
		if (BrushWorld)
		{
			Brush->bInManipulation = PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive;
			Build(BrushWorld, Brush);
		}
	}
}