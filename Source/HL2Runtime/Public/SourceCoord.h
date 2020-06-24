#pragma once

#include "CoreMinimal.h"

// Provides methods for converting coordinates from one coordinate space to another
class FSourceCoord
{
private:

	const FMatrix matrix;
	const FTransform transform;
	const bool reverseWinding;

public:

	inline FSourceCoord(const FVector& unitX, const FVector& unitY, const FVector& unitZ);

public:

	inline bool ShouldReverseWinding() const;

	inline const FMatrix& GetMatrix() const;

	inline const FTransform& GetTransform() const;
	
	inline FVector Position(const FVector& inPosition) const;

	inline FVector Direction(const FVector& inDirection) const;

	inline FRotator Rotator(const FRotator& inRotator) const;

	inline FQuat Quat(const FQuat& inQuat) const;

	inline FTransform Transform(const FTransform& inTransform) const;

	inline FPlane Plane(const FPlane& inPlane) const;

};

inline FSourceCoord::FSourceCoord(const FVector& unitX, const FVector& unitY, const FVector& unitZ)
	: matrix(FMatrix(unitX, unitY, unitZ, FVector::ZeroVector))
	, transform(matrix)
	, reverseWinding(matrix.Determinant() < 0.0f)
{ }

inline bool FSourceCoord::ShouldReverseWinding() const
{
	return reverseWinding;
}

inline const FMatrix& FSourceCoord::GetMatrix() const
{
	return matrix;
}

inline const FTransform& FSourceCoord::GetTransform() const
{
	return transform;
}

inline FVector FSourceCoord::Position(const FVector& inPosition) const
{
	return transform.TransformPosition(inPosition);
}

inline FVector FSourceCoord::Direction(const FVector& inDirection) const
{
	return transform.TransformVector(inDirection).GetSafeNormal();
}

inline FRotator FSourceCoord::Rotator(const FRotator& inRotator) const
{
	return Quat(FQuat(inRotator)).Rotator();
}

inline FQuat FSourceCoord::Quat(const FQuat& inQuat) const
{
	return transform.TransformRotation(inQuat);
}

inline FTransform FSourceCoord::Transform(const FTransform& inTransform) const
{
	FTransform outTransform = inTransform * transform;
	outTransform.SetScale3D(inTransform.GetScale3D());
	return outTransform;
}

inline FPlane FSourceCoord::Plane(const FPlane& inPlane) const
{
	return inPlane.TransformBy(matrix);
}

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

const FSourceCoord UnrealToSource(
	FVector(0.0f, 1.0f, 0.0f), // map unreal +X to source +Y axis
	FVector(1.0f, 0.0f, 0.0f), // map unreal +Y to source +X axis
	FVector(0.0f, 0.0f, 1.0f)  // map unreal +Z to source +Z axis
);
const FSourceCoord UnrealToStudioMdl(
	FVector(1.0f, 0.0f, 0.0f),
	FVector(0.0f, -1.0f, 0.0f),
	FVector(0.0f, 0.0f, 1.0f)
);