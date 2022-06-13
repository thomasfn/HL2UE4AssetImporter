#include "SkeletonUtils.h"

// Get transform of bone relative to the skeleton (e.g. component space), rather than relative to the parent bone
FTransform3f FSkeletonUtils::GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex)
{
	const FTransform3f boneRelTransform(skeleton.GetRefBonePose()[boneIndex]);
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

TArray<FTransform3f> FSkeletonUtils::GetSkeletonComponentTransforms(const FReferenceSkeleton& skeleton)
{
	TArray<FTransform3f> result;
	result.AddUninitialized(skeleton.GetRawBoneNum());
	for (int i = 0; i < skeleton.GetRawBoneNum(); ++i)
	{
		const int parentBoneIndex = skeleton.GetParentIndex(i);
		check(parentBoneIndex < i);
		const FTransform3f& parentComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? result[parentBoneIndex] : FTransform3f::Identity;
		const FTransform3f childLocalTransform(skeleton.GetRefBonePose()[i]);
		result[i] = childLocalTransform * parentComponentTransform;
	}
	return result;
}

// Set transform of bone relative to the skeleton (e.g. component space), rather than relative to the parent bone
void FSkeletonUtils::SetBoneComponentTransform(FReferenceSkeletonModifier& skeletonModifier, int boneIndex, const FTransform3f& transform)
{
	const FReferenceSkeleton& skeleton = skeletonModifier.GetReferenceSkeleton();
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	if (!skeleton.IsValidIndex(parentBoneIndex))
	{
		skeletonModifier.UpdateRefPoseTransform(boneIndex, FTransform(transform));
	}
	else
	{
		const FTransform3f& parentBoneComponentTransform = GetBoneComponentTransform(skeleton, parentBoneIndex);
		return skeletonModifier.UpdateRefPoseTransform(boneIndex, FTransform(transform.GetRelativeTransform(parentBoneComponentTransform)));
	}
}

void FSkeletonUtils::SetBoneComponentTransform(FReferenceSkeleton& skeleton, int boneIndex, const FTransform3f& transform)
{
	FReferenceSkeletonModifier modifier(skeleton, nullptr);
	SetBoneComponentTransform(modifier, boneIndex, transform);
}

void FSkeletonUtils::SetBoneComponentTransforms(FReferenceSkeletonModifier& skeletonModifier, const TArray<FTransform3f>& transforms)
{
	const FReferenceSkeleton& skeleton = skeletonModifier.GetReferenceSkeleton();
	for (int i = 0; i < transforms.Num(); ++i)
	{
		const int parentBoneIndex = skeleton.GetParentIndex(i);
		check(parentBoneIndex < i);
		const FTransform3f& parentComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? transforms[parentBoneIndex] : FTransform3f::Identity;
		const FTransform3f& childLocalTransform = transforms[i].GetRelativeTransform(parentComponentTransform);
		skeletonModifier.UpdateRefPoseTransform(i, FTransform(childLocalTransform));
	}
}

void FSkeletonUtils::SetBoneComponentTransforms(FReferenceSkeleton& skeleton, const TArray<FTransform3f>& transforms)
{
	FReferenceSkeletonModifier modifier(skeleton, nullptr);
	SetBoneComponentTransforms(modifier, transforms);
}

FTransform3f FSkeletonUtils::GetTrackBoneLocalTransform(const FRawAnimSequenceTrack& track, int frameIndex)
{
	FTransform3f localBonePose = FTransform3f::Identity;
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

void FSkeletonUtils::SetTrackBoneLocalTransform(FRawAnimSequenceTrack& track, int frameIndex, const FTransform3f& transform)
{
	const FTransform3f old = GetTrackBoneLocalTransform(track, frameIndex);
	const FVector3f oldPos = old.GetLocation();
	const FQuat4f oldRot = old.GetRotation();
	const FVector3f oldScale = old.GetScale3D();
	const FVector3f newPos = transform.GetLocation();
	const FQuat4f newRot = transform.GetRotation();
	const FVector3f newScale = transform.GetScale3D();
	if (!oldPos.Equals(newPos) || track.PosKeys.Num() == 0)
	{
		if (track.PosKeys.Num() == 0) { track.PosKeys.Add(FVector3f::ZeroVector); }
		while (track.PosKeys.Num() <= frameIndex)
		{
			const FVector3f last = track.PosKeys.Last();
			track.PosKeys.Add(last);
		}
		track.PosKeys[frameIndex] = newPos;
	}
	if (!oldRot.Equals(newRot) || track.RotKeys.Num() == 0)
	{
		if (track.RotKeys.Num() == 0) { track.RotKeys.Add(FQuat4f::Identity); }
		while (track.RotKeys.Num() <= frameIndex)
		{
			const FQuat4f last = track.RotKeys.Last();
			track.RotKeys.Add(last);
		}
		track.RotKeys[frameIndex] = newRot;
	}
	if (!oldScale.Equals(newScale))
	{
		if (track.ScaleKeys.Num() == 0) { track.ScaleKeys.Add(FVector3f::OneVector); }
		while (track.ScaleKeys.Num() <= frameIndex)
		{
			const FVector3f last = track.ScaleKeys.Last();
			track.ScaleKeys.Add(last);
		}
		track.ScaleKeys[frameIndex] = newScale;
	}
}

FTransform3f FSkeletonUtils::GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	const FTransform3f& parentBoneComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? GetBoneComponentTransform(skeleton, parentBoneIndex, trackMap, frameIndex) : FTransform3f::Identity;
	const FRawAnimSequenceTrack* trackPtr = trackMap.Find((uint8)boneIndex);
	const FTransform3f boneLocalTransform = trackPtr != nullptr ? GetTrackBoneLocalTransform(*trackPtr, frameIndex) : FTransform3f(skeleton.GetRefBonePose()[boneIndex]);
	return boneLocalTransform * parentBoneComponentTransform;
}

FTransform3f FSkeletonUtils::TransformBoneComponentTransform(const FTransform3f& boneComponentTransform, const FSourceCoord& transform)
{
	return transform.Transform(boneComponentTransform);
}

FReferenceSkeleton FSkeletonUtils::TransformSkeleton(const FReferenceSkeleton& skeleton, const FSourceCoord& transform)
{
	TArray<FTransform3f> componentTransforms = GetSkeletonComponentTransforms(skeleton);
	TArray<FTransform3f> transformedComponentTransforms;
	transformedComponentTransforms.AddUninitialized(componentTransforms.Num());
	for (int i = 0; i < skeleton.GetRawBoneNum(); ++i)
	{
		transformedComponentTransforms[i] = TransformBoneComponentTransform(componentTransforms[i], transform);
	}
	FReferenceSkeleton transformedSkeleton = skeleton;
	SetBoneComponentTransforms(transformedSkeleton, transformedComponentTransforms);
	return transformedSkeleton;
}

void FSkeletonUtils::MergeAnimTracks(FReferenceSkeleton& skeleton, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	FReferenceSkeletonModifier modifier(skeleton, nullptr);
	for (const TPair<uint8, FRawAnimSequenceTrack>& pair : trackMap)
	{
		modifier.UpdateRefPoseTransform(pair.Key, FTransform(GetTrackBoneLocalTransform(pair.Value, frameIndex)));
	}
}

void FSkeletonUtils::ExtractAnimTracks(const FReferenceSkeleton& skeleton, TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	for (TPair<uint8, FRawAnimSequenceTrack>& pair : trackMap)
	{
		SetTrackBoneLocalTransform(pair.Value, frameIndex, FTransform3f(skeleton.GetRefBonePose()[pair.Key]));
	}
}