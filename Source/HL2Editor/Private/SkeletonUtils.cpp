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

void FSkeletonUtils::SetTrackBoneLocalTransform(FRawAnimSequenceTrack& track, int frameIndex, const FTransform& transform)
{
	const FTransform old = GetTrackBoneLocalTransform(track, frameIndex);
	const FVector oldPos = old.GetLocation();
	const FQuat oldRot = old.GetRotation();
	const FVector oldScale = old.GetScale3D();
	const FVector newPos = transform.GetLocation();
	const FQuat newRot = transform.GetRotation();
	const FVector newScale = transform.GetScale3D();
	if (!oldPos.Equals(newPos))
	{
		if (track.PosKeys.Num() == 0) { track.PosKeys.Add(FVector::ZeroVector); }
		while (track.PosKeys.Num() <= frameIndex)
		{
			const FVector last = track.PosKeys.Last();
			track.PosKeys.Add(last);
		}
		track.PosKeys[frameIndex] = newPos;
	}
	if (!oldRot.Equals(newRot))
	{
		if (track.RotKeys.Num() == 0) { track.RotKeys.Add(FQuat::Identity); }
		while (track.RotKeys.Num() <= frameIndex)
		{
			const FQuat last = track.RotKeys.Last();
			track.RotKeys.Add(last);
		}
		track.RotKeys[frameIndex] = newRot;
	}
	if (!oldScale.Equals(newScale))
	{
		if (track.ScaleKeys.Num() == 0) { track.ScaleKeys.Add(FVector::OneVector); }
		while (track.ScaleKeys.Num() <= frameIndex)
		{
			const FVector last = track.ScaleKeys.Last();
			track.ScaleKeys.Add(last);
		}
		track.ScaleKeys[frameIndex] = newScale;
	}
}

FTransform FSkeletonUtils::GetBoneComponentTransform(const FReferenceSkeleton& skeleton, int boneIndex, const TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	const int parentBoneIndex = skeleton.GetParentIndex(boneIndex);
	const FTransform& parentBoneComponentTransform = skeleton.IsValidIndex(parentBoneIndex) ? GetBoneComponentTransform(skeleton, parentBoneIndex, trackMap, frameIndex) : FTransform::Identity;
	const FRawAnimSequenceTrack* trackPtr = trackMap.Find((uint8)boneIndex);
	const FTransform boneLocalTransform = trackPtr != nullptr ? GetTrackBoneLocalTransform(*trackPtr, frameIndex) : skeleton.GetRefBonePose()[boneIndex];
	return boneLocalTransform * parentBoneComponentTransform;
}

FTransform FSkeletonUtils::TransformBoneComponentTransform(const FTransform& boneComponentTransform, const FSourceCoord& transform)
{
	return FTransform(
		transform.Quat(boneComponentTransform.GetRotation()),
		transform.Position(boneComponentTransform.GetTranslation()),
		boneComponentTransform.GetScale3D()
	);
}

FReferenceSkeleton FSkeletonUtils::TransformSkeleton(const FReferenceSkeleton& skeleton, const FSourceCoord& transform)
{
	TArray<FTransform> componentTransforms = GetSkeletonComponentTransforms(skeleton);
	TArray<FTransform> transformedComponentTransforms;
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
		modifier.UpdateRefPoseTransform(pair.Key, GetTrackBoneLocalTransform(pair.Value, frameIndex));
	}
}

void FSkeletonUtils::ExtractAnimTracks(const FReferenceSkeleton& skeleton, TMap<uint8, FRawAnimSequenceTrack>& trackMap, int frameIndex)
{
	for (TPair<uint8, FRawAnimSequenceTrack>& pair : trackMap)
	{
		SetTrackBoneLocalTransform(pair.Value, frameIndex, skeleton.GetRefBonePose()[pair.Key]);
	}
}