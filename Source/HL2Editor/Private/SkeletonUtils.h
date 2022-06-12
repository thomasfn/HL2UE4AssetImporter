#pragma once

#include "CoreMinimal.h"

#include "SourceCoord.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"

/* Contains utilities for manipulation skeletons and animation tracks. */
class FSkeletonUtils
{
public:

	// Retrieves the transform of the specified bone in component space (given a skeleton where all transforms are in parent bone space)
	static FTransform3f GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex);

	// Retrieves the transform of all bones in component space (given a skeleton where all transforms are in parent bone space)
	static TArray<FTransform3f> GetSkeletonComponentTransforms(const FReferenceSkeleton& skeleton);

	// Sets the transform of the specified bone in component space (given a skeleton where all transforms are in parent bone space)
	static void SetBoneComponentTransform(FReferenceSkeletonModifier& skeletonModifier, int boneIndex, const FTransform3f& transform);

	// Sets the transform of the specified bone in component space (given a skeleton where all transforms are in parent bone space)
	static void SetBoneComponentTransform(FReferenceSkeleton& skeleton, int boneIndex, const FTransform3f& transform);

	// Sets the transform of all bones in component space (given a skeleton where all transforms are in parent bone space)
	static void SetBoneComponentTransforms(FReferenceSkeletonModifier& skeletonModifier, const TArray<FTransform3f>& transforms);

	// Sets the transform of all bones in component space (given a skeleton where all transforms are in parent bone space)
	static void SetBoneComponentTransforms(FReferenceSkeleton& skeleton, const TArray<FTransform3f>& transforms);

	// Retrieves the transform of a bone at the specified frame index from the animation track
	static FTransform3f GetTrackBoneLocalTransform(const FRawAnimSequenceTrack& track, int frameIndex);

	// Sets the transform of a bone at the specified frame index from the animation track
	static void SetTrackBoneLocalTransform(FRawAnimSequenceTrack& track, int frameIndex, const FTransform3f& transform);

	static FTransform3f GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex);

	// Given the transform of a bone in component space, moves it to a different coordinate system via an FSourceCoord (but still keeps it in component space)
	static FTransform3f TransformBoneComponentTransform(const FTransform3f& boneComponentTransform, const FSourceCoord& transform);

public:

	// Moves an entire skeleton to a different coordinate system via an FSourceCoord
	static FReferenceSkeleton TransformSkeleton(const FReferenceSkeleton& skeleton, const FSourceCoord& transform);

	// Merges the transforms from a specific frame of a set of animation tracks to a skeleton
	static void MergeAnimTracks(FReferenceSkeleton& skeleton, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex);

	// Extracts the transforms from a skeleton to a specific frame of a set of animation tracks
	static void ExtractAnimTracks(const FReferenceSkeleton& skeleton, TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex);
	
};