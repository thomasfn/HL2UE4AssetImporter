#pragma once

#include "CoreMinimal.h"

#include "SourceCoord.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"

/**  */
class FSkeletonUtils
{
public:

	static FTransform GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex);

	static TArray<FTransform> GetSkeletonComponentTransforms(const FReferenceSkeleton& skeleton);

	static void SetBoneComponentTransform(FReferenceSkeletonModifier& skeletonModifier, int boneIndex, const FTransform& transform);

	static void SetBoneComponentTransform(FReferenceSkeleton& skeleton, int boneIndex, const FTransform& transform);

	static void SetBoneComponentTransforms(FReferenceSkeletonModifier& skeletonModifier, const TArray<FTransform>& transforms);

	static void SetBoneComponentTransforms(FReferenceSkeleton& skeleton, const TArray<FTransform>& transforms);

	static FTransform GetTrackBoneLocalTransform(const FRawAnimSequenceTrack& track, int frameIndex);

	static FTransform GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex);

public:


	static FReferenceSkeleton TransformSkeleton(const FReferenceSkeleton& skeleton, const FSourceCoord& transform);
	
};