#pragma once

#include "CoreMinimal.h"

// Provides methods for converting coordinates from one coordinate space to another
class FSourceCoord
{
private:

	const FVector unitX, unitY, unitZ;
	const bool reverseWinding;

public:

	inline FSourceCoord(const FVector& unitX, const FVector& unitY, const FVector& unitZ);

private:

	inline float Determinant() const;

	inline FVector Multiply(const FVector& vec) const;

public:

	inline bool ShouldReverseWinding() const;
	
	inline FVector Position(const FVector& position) const;

	inline FVector Direction(const FVector& direction) const;

	inline FVector Scale(const FVector& scale) const;

	inline FRotator Rotator(const FRotator& rotator) const;

	inline FQuat Quat(const FQuat& quat) const;

	inline FTransform Transform(const FTransform& transform) const;

	inline FPlane Plane(const FPlane& plane) const;

};

inline FSourceCoord::FSourceCoord(const FVector& unitX, const FVector& unitY, const FVector& unitZ)
	: unitX(unitX)
	, unitY(unitY)
	, unitZ(unitZ)
	, reverseWinding(Determinant() < 0.0f)
{ }

inline float FSourceCoord::Determinant() const
{
	return
		unitX.X * (unitY.Y * unitZ.Z - unitY.Z * unitZ.Y) -
		unitX.Y * (unitY.X * unitZ.Z - unitY.Z * unitZ.X) +
		unitX.Z * (unitY.X * unitZ.Y - unitY.Y * unitZ.X);
}

inline FVector FSourceCoord::Multiply(const FVector& vec) const
{
	return FVector(
		FVector::DotProduct(vec, unitX),
		FVector::DotProduct(vec, unitY),
		FVector::DotProduct(vec, unitZ)
	);
}

inline bool FSourceCoord::ShouldReverseWinding() const
{
	return reverseWinding;
}

inline FVector FSourceCoord::Position(const FVector& position) const
{
	return Multiply(position);
}

inline FVector FSourceCoord::Direction(const FVector& direction) const
{
	return Multiply(direction);
}

inline FVector FSourceCoord::Scale(const FVector& scale) const
{
	return Multiply(scale);
}

inline FRotator FSourceCoord::Rotator(const FRotator& rotator) const
{
	return Quat(FQuat(rotator)).Rotator();
}

inline FQuat FSourceCoord::Quat(const FQuat& quat) const
{
	return FQuat::FindBetweenNormals(FVector::ForwardVector, Direction(quat.RotateVector(FVector::ForwardVector)));
}

inline FTransform FSourceCoord::Transform(const FTransform& transform) const
{
	return FTransform(
		Quat(transform.GetRotation()),
		Position(transform.GetTranslation()),
		Scale(transform.GetScale3D())
	);
}

inline FPlane FSourceCoord::Plane(const FPlane& plane) const
{
	return FPlane(
		Direction(plane.GetUnsafeNormal()),
		plane.W
	);
}

static const FMatrix sourceToUnrealMatrix = FMatrix(
	FPlane(0.0f, 1.0f, 0.0f, 0.0f),
	FPlane(1.0f, 0.0f, 0.0f, 0.0f),
	FPlane(0.0f, 0.0f, 1.0f, 0.0f),
	FPlane(0.0f, 0.0f, 0.0f, 1.0f)
);
static const FMatrix studioMdlToUnrealMatrix = FMatrix(
	FPlane(1.0f, 0.0f, 0.0f, 0.0f),
	FPlane(0.0f, 1.0f, 0.0f, 0.0f),
	FPlane(0.0f, 0.0f, 1.0f, 0.0f),
	FPlane(0.0f, 0.0f, 0.0f, 1.0f)
);

const FSourceCoord SourceToUnreal(
	FVector(0.0f, 1.0f, 0.0f), // map source +X to unreal +Y axis
	FVector(1.0f, 0.0f, 0.0f), // map source +Y to unreal +X axis
	FVector(0.0f, 0.0f, 1.0f)  // map source +Z to unreal +Z axis
);
const FSourceCoord StudioMdlToUnreal(
	FVector(1.0f, 0.0f, 0.0f),
	FVector(0.0f, -1.0f, 0.0f),
	FVector(0.0f, 0.0f, 1.0f) 
);