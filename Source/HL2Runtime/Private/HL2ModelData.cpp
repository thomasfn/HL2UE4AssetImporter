#include "HL2ModelData.h"

#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

bool UHL2ModelData::ApplyBodygroupToStaticMesh(UStaticMeshComponent* target, const FName bodygroupName, int index)
{
	const FModelBodygroup* bodygroup = Bodygroups.Find(bodygroupName);
	if (bodygroup == nullptr) { return false; }
	if (!bodygroup->Mappings.IsValidIndex(index)) { return false; }
	const FModelBodygroupMapping& mapping = bodygroup->Mappings[index];
	const int lodCount = target->GetStaticMesh()->GetNumLODs();
	for (const int sectionIndex : bodygroup->AllSections)
	{
		for (int lodIndex = 0; lodIndex < lodCount; ++lodIndex)
		{
			// TODO: Determine how to show/hide sections!
			//target->ShowMaterialSection(sectionIndex, mapping.Sections.Contains(sectionIndex), lodIndex);
		}
	}
	return true;
}

bool UHL2ModelData::ApplySkinToStaticMesh(UStaticMeshComponent* target, int index)
{
	if (!Skins.IsValidIndex(index)) { return false; }
	const FModelSkinMapping& mapping = Skins[index];
	for (const auto& pair : mapping.MaterialOverrides)
	{
		target->SetMaterial(pair.Key, pair.Value);
	}
	return true;
}

bool UHL2ModelData::ApplyBodygroupToSkeletalMesh(USkeletalMeshComponent* target, const FName bodygroupName, int index)
{
	const FModelBodygroup* bodygroup = Bodygroups.Find(bodygroupName);
	if (bodygroup == nullptr) { return false; }
	if (!bodygroup->Mappings.IsValidIndex(index)) { return false; }
	const FModelBodygroupMapping& mapping = bodygroup->Mappings[index];
	const int lodCount = target->SkeletalMesh->GetLODNum();
	for (const int sectionIndex : bodygroup->AllSections)
	{
		for (int lodIndex = 0; lodIndex < lodCount; ++lodIndex)
		{
			target->ShowMaterialSection(0, sectionIndex, mapping.Sections.Contains(sectionIndex), lodIndex);
		}
	}
	return true;
}

bool UHL2ModelData::ApplySkinToSkeletalMesh(USkeletalMeshComponent* target, int index)
{
	if (!Skins.IsValidIndex(index)) { return false; }
	const FModelSkinMapping& mapping = Skins[index];
	for (const auto& pair : mapping.MaterialOverrides)
	{
		target->SetMaterial(pair.Key, pair.Value);
	}
	return true;
}