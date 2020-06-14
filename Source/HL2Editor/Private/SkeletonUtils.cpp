#include "SkeletonUtils.h"

// Get transform of bone relative to the skeleton (e.g. component space), rather than relative to the parent bone
FTransform FSkeletonUtils::GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex)
{
	const FTransform& boneRelTransform = skeleton.GetRefBonePose()[boneIndex];
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	if (!skeleton.IsValidIndex(parentBoneIndex))
	{
		return boneRelTransform;
	}
	else
	{
		return boneRelTransform * GetBoneComponentTransform(skeleton, parentBoneIndex);
	}
}

TArray<FTransform> FSkeletonUtils::GetSkeletonComponentTransforms(const FReferenceSkeleton& skeleton)
{
	TArray<FTransform> result;
	result.AddUninitialized(skeleton.GetRawBoneNum());
	for (int i = 0; i < skeleton.GetRawBoneNum(); ++i)
	{
		const int parentBoneIndex = skeleton.GetParentIndex(i);
		check(parentBoneIndex < i);
		const FTransform& parentComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? result[parentBoneIndex] : FTransform::Identity;
		const FTransform& childLocalTransform = skeleton.GetRefBonePose()[i];
		result[i] = childLocalTransform * parentComponentTransform;
	}
	return result;
}

// Set transform of bone relative to the skeleton (e.g. component space), rather than relative to the parent bone
void FSkeletonUtils::SetBoneComponentTransform(FReferenceSkeletonModifier& skeletonModifier, int boneIndex, const FTransform& transform)
{
	const FReferenceSkeleton& skeleton = skeletonModifier.GetReferenceSkeleton();
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	if (!skeleton.IsValidIndex(parentBoneIndex))
	{
		skeletonModifier.UpdateRefPoseTransform(boneIndex, transform);
	}
	else
	{
		const FTransform& parentBoneComponentTransform = GetBoneComponentTransform(skeleton, parentBoneIndex);
		return skeletonModifier.UpdateRefPoseTransform(boneIndex, transform.GetRelativeTransform(parentBoneComponentTransform));
	}
}

void FSkeletonUtils::SetBoneComponentTransform(FReferenceSkeleton& skeleton, int boneIndex, const FTransform& transform)
{
	FReferenceSkeletonModifier modifier(skeleton, nullptr);
	SetBoneComponentTransform(modifier, boneIndex, transform);
}

void FSkeletonUtils::SetBoneComponentTransforms(FReferenceSkeletonModifier& skeletonModifier, const TArray<FTransform>& transforms)
{
	const FReferenceSkeleton& skeleton = skeletonModifier.GetReferenceSkeleton();
	for (int i = 0; i < transforms.Num(); ++i)
	{
		const int parentBoneIndex = skeleton.GetParentIndex(i);
		check(parentBoneIndex < i);
		const FTransform& parentComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? transforms[parentBoneIndex] : FTransform::Identity;
		const FTransform& childLocalTransform = transforms[i].GetRelativeTransform(parentComponentTransform);
		skeletonModifier.UpdateRefPoseTransform(i, childLocalTransform);
	}
}

void FSkeletonUtils::SetBoneComponentTransforms(FReferenceSkeleton& skeleton, const TArray<FTransform>& transforms)
{
	FReferenceSkeletonModifier modifier(skeleton, nullptr);
	SetBoneComponentTransforms(modifier, transforms);
}

FTransform FSkeletonUtils::GetTrackBoneLocalTransform(const FRawAnimSequenceTrack& track, int frameIndex)
{
	FTransform localBonePose = FTransform::Identity;
	if (track.PosKeys.Num() > 0)
	{
		localBonePose.SetTranslation(track.PosKeys[frameIndex >= track.PosKeys.Num() ? track.PosKeys.Num() - 1 : frameIndex]);
	}
	if (track.RotKeys.Num() > 0)
	{
		localBonePose.SetRotation(track.RotKeys[frameIndex >= track.RotKeys.Num() ? track.RotKeys.Num() - 1 : frameIndex]);
	}
	if (track.ScaleKeys.Num() > 0)
	{
		localBonePose.SetScale3D(track.ScaleKeys[frameIndex >= track.ScaleKeys.Num() ? track.ScaleKeys.Num() - 1 : frameIndex]);
	}
	return localBonePose;
}

FTransform FSkeletonUtils::GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	const FTransform& parentBoneComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? GetBoneComponentTransform(skeleton, parentBoneIndex, trackMap, frameIndex) : FTransform::Identity;
	const FRawAnimSequenceTrack* trackPtr = trackMap.Find((uint8)boneIndex);
	const FTransform boneLocalTransform = trackPtr != nullptr ? GetTrackBoneLocalTransform(*trackPtr, frameIndex) : skeleton.GetRefBonePose()[boneIndex];
	return boneLocalTransform * parentBoneComponentTransform;
}

FReferenceSkeleton FSkeletonUtils::TransformSkeleton(const FReferenceSkeleton& skeleton, const FSourceCoord& transform)
{
	TArray<FTransform> componentTransforms = GetSkeletonComponentTransforms(skeleton);
	TArray<FTransform> transformedComponentTransforms;
	transformedComponentTransforms.AddUninitialized(componentTransforms.Num());
	FReferenceSkeleton dstSkeleton = skeleton;
	for (int i = 0; i < skeleton.GetRawBoneNum(); ++i)
	{
		const FTransform& boneComponentTransform = componentTransforms[i];
		const FTransform boneTransformedComponent(
			transform.Quat(boneComponentTransform.GetRotation()),
			transform.Position(boneComponentTransform.GetTranslation()),
			boneComponentTransform.GetScale3D()
		);
		transformedComponentTransforms[i] = boneTransformedComponent;
	}
	SetBoneComponentTransforms(dstSkeleton, transformedComponentTransforms);
	return dstSkeleton;
}