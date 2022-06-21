#pragma once

#include "CoreMinimal.h"

// Provides methods for converting coordinates from one coordinate space to another
class FSourceCoord
{
private:

	const FMatrix44f matrix;
	const FTransform3f transform;
	const bool reverseWinding;

public:

	inline FSourceCoord(const FVector3f& unitX, const FVector3f& unitY, const FVector3f& unitZ);

public:

	inline bool ShouldReverseWinding() const;

	inline const FMatrix44f& GetMatrix() const;

	inline const FTransform3f& GetTransform() const;
	
	inline FVector3f Position(const FVector3f& inPosition) const;

	inline FVector3f Direction(const FVector3f& inDirection) const;

	inline FRotator3f Rotator(const FRotator3f& inRotator) const;

	inline FQuat4f Quat(const FQuat4f& inQuat) const;

	inline FTransform3f Transform(const FTransform3f& inTransform) const;

	inline FPlane4f Plane(const FPlane4f& inPlane) const;

};

inline FSourceCoord::FSourceCoord(const FVector3f& unitX, const FVector3f& unitY, const FVector3f& unitZ)
	: matrix(FMatrix44f(unitX, unitY, unitZ, FVector3f::ZeroVector))
	, transform(matrix)
	, reverseWinding(matrix.Determinant() < 0.0f)
{ }

inline bool FSourceCoord::ShouldReverseWinding() const
{
	return reverseWinding;
}

inline const FMatrix44f& FSourceCoord::GetMatrix() const
{
	return matrix;
}

inline const FTransform3f& FSourceCoord::GetTransform() const
{
	return transform;
}

inline FVector3f FSourceCoord::Position(const FVector3f& inPosition) const
{
	return transform.TransformPosition(inPosition);
}

inline FVector3f FSourceCoord::Direction(const FVector3f& inDirection) const
{
	return transform.TransformVector(inDirection).GetSafeNormal();
}

inline FRotator3f FSourceCoord::Rotator(const FRotator3f& inRotator) const
{
	return Quat(FQuat4f(inRotator)).Rotator();
}

inline FQuat4f FSourceCoord::Quat(const FQuat4f& inQuat) const
{
	return transform.TransformRotation(inQuat);
}

inline FTransform3f FSourceCoord::Transform(const FTransform3f& inTransform) const
{
	FTransform3f outTransform = inTransform * transform;
	outTransform.SetScale3D(inTransform.GetScale3D());
	return outTransform;
}

inline FPlane4f FSourceCoord::Plane(const FPlane4f& inPlane) const
{
	return inPlane.TransformBy(matrix);
}

constexpr float SOURCE_UNIT_SCALE = 2.54f; // inches to cm

const FSourceCoord SourceToUnreal(
	FVector3f(0.0f, 1.0f, 0.0f) * SOURCE_UNIT_SCALE, // map source +X to unreal +Y axis
	FVector3f(1.0f, 0.0f, 0.0f) * SOURCE_UNIT_SCALE, // map source +Y to unreal +X axis
	FVector3f(0.0f, 0.0f, 1.0f) * SOURCE_UNIT_SCALE  // map source +Z to unreal +Z axis
);
const FSourceCoord StudioMdlToUnreal(
	FVector3f(1.0f, 0.0f, 0.0f) * SOURCE_UNIT_SCALE, // map studiomodel +X to unreal +X axis
	FVector3f(0.0f, -1.0f, 0.0f) * SOURCE_UNIT_SCALE, // map studiomodel +Y to unreal -Y axis
	FVector3f(0.0f, 0.0f, 1.0f) * SOURCE_UNIT_SCALE // map studiomodel +Z to unreal -Z axis
);

const FSourceCoord UnrealToSource(
	FVector3f(0.0f, 1.0f, 0.0f) / SOURCE_UNIT_SCALE, // map unreal +X to source +Y axis
	FVector3f(1.0f, 0.0f, 0.0f) / SOURCE_UNIT_SCALE, // map unreal +Y to source +X axis
	FVector3f(0.0f, 0.0f, 1.0f) / SOURCE_UNIT_SCALE  // map unreal +Z to source +Z axis
);
const FSourceCoord UnrealToStudioMdl(
	FVector3f(1.0f, 0.0f, 0.0f) / SOURCE_UNIT_SCALE,
	FVector3f(0.0f, -1.0f, 0.0f) / SOURCE_UNIT_SCALE,
	FVector3f(0.0f, 0.0f, 1.0f) / SOURCE_UNIT_SCALE
);