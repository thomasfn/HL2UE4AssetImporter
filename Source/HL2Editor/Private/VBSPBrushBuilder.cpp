#include "HL2EditorPrivatePCH.h"

#include "VBSPBrushBuilder.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Engine/Selection.h"
#include "Engine/Polys.h"
#include "SnappingUtils.h"
//#include "BSPOps.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Vector2D.h"
#include "IHL2Runtime.h"

DEFINE_LOG_CATEGORY(LogVBSPBrushBuilder);

const float UVBSPBrushBuilder::snapThreshold = 1.0f / 128.0f;
const float UVBSPBrushBuilder::containsThreshold = 1.0f / 256.0f;
const float UVBSPBrushBuilder::lineParallelThreshold = 1.0f / 256.0f;

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
	//FRotator temp = FRotator::ZeroRotator;
	//FSnappingUtils::SnapToBSPVertex(V, FVector::ZeroVector, temp);
	return Vertices.AddUnique(V);
}

int32 UVBSPBrushBuilder::Vertex3f(float X, float Y, float Z)
{
	return Vertexv(FVector(X, Y, Z));
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

FVector UVBSPBrushBuilder::EvaluateGeometricCenter() const
{
	FVector intersectionSum = FVector::ZeroVector;
	int intersectionCnt = 0;
	for (int i = 0; i < Planes.Num(); ++i)
	{
		for (int j = i + 1; j < Planes.Num(); ++j)
		{
			for (int k = j + 1; k < Planes.Num(); ++k)
			{
				FVector tmp;
				if (FMath::IntersectPlanes3(tmp, Planes[i], Planes[j], Planes[k]))
				{
					intersectionSum += tmp;
					++intersectionCnt;
				}
			}
		}
	}
	return SnapPoint(intersectionSum / intersectionCnt);
}

void UVBSPBrushBuilder::BuildVBSP()
{
	const int planeCnt = Planes.Num();

	// Iterate all planes
	for (int i = 0; i < planeCnt; ++i)
	{
		const FPlane& plane = Planes[i];

		TArray<TTuple<FVector, FVector>> lines;
		TArray<FVector> points;

		// Iterate all other planes
		for (int j = 0; j < planeCnt; ++j)
		{
			if (j != i)
			{
				// Intersect both planes, if it produces a line then emit that
				const FPlane& otherPlane = Planes[j];
				FVector vI, vD;
				if (FMath::IntersectPlanes2(vI, vD, plane, otherPlane))
				{
					lines.Add(TTuple<FVector, FVector>(vI, vD));
				}
			}
		}

		// Iterate all lines
		for (const auto& line : lines)
		{
			// Iterate all other planes
			for (const FPlane& otherPlane : Planes)
			{
				// Intersect line with plane, if it produces a point then emit that but only if the point is inside the brush
				const float d = FVector::DotProduct(line.Value, otherPlane);
				if (FMath::Abs(d) > lineParallelThreshold)
				{
					FVector point = FMath::LinePlaneIntersection(line.Key, line.Key + line.Value, otherPlane);
					if (ContainsPoint(point))
					{
						points.AddUnique(point);
					}
				}
			}
		}

		// Commit the points as a poly
		if (CommitPoly(plane, points))
		{
			UE_LOG(LogVBSPBrushBuilder, Log, TEXT("- Built plane %d into a poly with %d verts"), i, Polys.Top().VertexIndices.Num());
		}
		else
		{
			UE_LOG(LogVBSPBrushBuilder, Log, TEXT("- Ignored plane %d"), i);
		}
	}
}

// isLeft(): test if a point is Left|On|Right of an infinite 2D line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 to P1
//          =0 for P2 on the line
//          <0 for P2 right of the line
inline float isLeft(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2)
{
	return (P1.X - P0.X) * (P2.Y - P0.Y) - (P2.X - P0.X) * (P1.Y - P0.Y);
}
//===================================================================

uint32 FindRightmostLowestPoint(const TArray<FVector2D>& points)
{
	if (points.Num() <= 1) { return 0; }
	uint32 bestPointIdx = 0;
	FVector2D bestPoint = points[bestPointIdx];
	for (uint32 i = 1; i < (uint32)points.Num(); ++i)
	{
		const FVector2D& pt = points[i];
		if (pt.Y < bestPoint.Y || (pt.Y == bestPoint.Y && pt.X > bestPoint.X))
		{
			bestPointIdx = i;
			bestPoint = pt;
		}
	}
	return bestPointIdx;
}


bool UVBSPBrushBuilder::CommitPoly(const FPlane& plane, const TArray<FVector>& points)
{
	// We can't make a poly unless we have at least 3 points
	if (points.Num() < 3) { return false; }

	// Find a quaternion that rotates the plane to Z=1
	// That way, all points lie on the Z=1 plane and we can reduce the problem to 2D
	FQuat quat = FQuat::FindBetweenNormals(plane, FVector::UpVector);
	TArray<FVector2D> localPoints;
	for (const FVector& point : points)
	{
		FVector transformed = quat * point;
		localPoints.Add(FVector2D(transformed.X, transformed.Y));
	}

	// Sort S by increasing X then by Y
	localPoints.Sort([](const FVector2D& LHS, const FVector2D& RHS)
		{
			if (LHS.X < RHS.X)
			{
				return true;
			}
			else if (LHS.X > RHS.X)
			{
				return false;
			}
			else
			{
				return LHS.Y < RHS.Y;
			}
		});

	// Derive convex hull
	TArray<FVector2D> convexHull;
	ChainHull2D(localPoints, convexHull);

	// Transform points back to original coord space, snap here too and trim duplicates
	TArray<FVector> polyPoints;
	FQuat invQuat = quat.Inverse();
	for (int i = 1; i < convexHull.Num(); ++i)
	{
		const FVector2D& localPt = convexHull[i];
		FVector transformed = FPlane::PointPlaneProject(invQuat * FVector(localPt.X, localPt.Y, 0.0f), plane);
		SnapPoint(transformed);
		polyPoints.AddUnique(transformed);
		
	}
	if (polyPoints.Num() < 3) { return false; }

	// Emit
	PolyBegin(1);
	for (const FVector& point : polyPoints)
	{
		Polyi(Vertexv(point));
	}
	PolyEnd();
	return true;
}


// chainHull_2D(): Andrew's monotone chain 2D convex hull algorithm
//     Input:  P[] = an array of 2D points 
//                  presorted by increasing x and y-coordinates
//     Output: H[] = an array of the convex hull vertices (max is n)
void UVBSPBrushBuilder::ChainHull2D(const TArray<FVector2D>& P, TArray<FVector2D>& H)
{
	// http://geomalgorithms.com/a10-_hull-1.html

	const int n = P.Num();

	// the output array H[] will be used as the stack
	int    bot = 0, top = (-1);   // indices for bottom and top of the stack
	int    i;                 // array scan index

	// Get the indices of points with min x-coord and min|max y-coord
	int minmin = 0, minmax;
	float xmin = P[0].X;
	for (i = 1; i < n; i++)
	{
		if (P[i].X != xmin) break;
	}
	minmax = i - 1;
	if (minmax == n - 1) // degenerate case: all x-coords == xmin
	{
		H.Push(P[minmin]);
		if (P[minmax].Y != P[minmin].Y) // a  nontrivial segment
		{
			H.Push(P[minmax]);
		}
		H.Push(P[minmin]);            // add polygon endpoint
		return;
	}

	// Get the indices of points with max x-coord and min|max y-coord
	int maxmin, maxmax = n - 1;
	float xmax = P[n - 1].X;
	for (i = n - 2; i >= 0; i--)
	{
		if (P[i].X != xmax) break;
	}
	maxmin = i + 1;

	// Compute the lower hull on the stack H
	H.Push(P[minmin]);      // push  minmin point onto stack
	i = minmax;
	while (++i <= maxmin)
	{
		// the lower line joins P[minmin]  with P[maxmin]
		if (isLeft(P[minmin], P[maxmin], P[i]) >= 0 && i < maxmin)
		{
			continue;           // ignore P[i] above or on the lower line
		}

		while (top > 0)         // there are at least 2 points on the stack
		{
			// test if  P[i] is left of the line at the stack top
			if (isLeft(H[top - 1], H[top], P[i]) > 0)
			{
				break;         // P[i] is a new hull  vertex
			}
			else
			{
				H.Pop();		// pop top point off  stack
			}
		}
		H.Push(P[i]);        // push P[i] onto stack
	}

	// Next, compute the upper hull on the stack H above  the bottom hull
	if (maxmax != maxmin)      // if  distinct xmax points
	{
		H.Push(P[maxmax]);  // push maxmax point onto stack
	}
	bot = top;                  // the bottom point of the upper hull stack
	i = maxmin;
	while (--i >= minmax)
	{
		// the upper line joins P[maxmax]  with P[minmax]
		if (isLeft(P[maxmax], P[minmax], P[i]) >= 0 && i > minmax)
		{
			continue;           // ignore P[i] below or on the upper line
		}

		while (top > bot)     // at least 2 points on the upper stack
		{
			// test if  P[i] is left of the line at the stack top
			if (isLeft(H[top - 1], H[top], P[i]) > 0)
			{
				break;         // P[i] is a new hull  vertex
			}
			else
			{
				H.Pop();         // pop top point off  stack
			}
		}
		H.Push(P[i]);        // push P[i] onto stack
	}
	if (minmax != minmin)
	{
		H.Push(P[minmin]);  // push  joining endpoint onto stack
	}
}

bool UVBSPBrushBuilder::ContainsPoint(const FVector& point, float threshold) const
{
	// The point must sit behind all planes to be considered inside the brush
	for (const auto& plane : Planes)
	{
		// If the distance is negative, it sits behind the plane
		const float dist = plane.PlaneDot(point);
		if (dist > threshold) return false;
	}
	return true;
}

void UVBSPBrushBuilder::SnapPoint(FVector& point)
{
	point.X = FMath::RoundToFloat(point.X / snapThreshold) * snapThreshold;
	point.Y = FMath::RoundToFloat(point.Y / snapThreshold) * snapThreshold;
	point.Z = FMath::RoundToFloat(point.Z / snapThreshold) * snapThreshold;
}

FVector UVBSPBrushBuilder::SnapPoint(const FVector& point)
{
	FVector pointCp = point;
	SnapPoint(pointCp);
	return pointCp;
}

bool UVBSPBrushBuilder::Build(UWorld* InWorld, ABrush* InBrush)
{
	if (Planes.Num() < 4)
		return BadParameters(LOCTEXT("VBSPNotEnoughPlanes", "Not enough planes in VBSP brush"));
	
	// TODO: Extra validation on planes? Check for unbound volume etc

	BeginBrush(false, GroupName);
	BuildVBSP();
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