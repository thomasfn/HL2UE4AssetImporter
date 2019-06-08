#include "HL2RuntimePrivatePCH.h"

#include "BaseEntity.h"

void ABaseEntity::InitialiseEntity_Implementation()
{
	if (!EntityData.Targetname.IsEmpty())
	{
		SetActorLabel(EntityData.Targetname);
	}
}