#include "HL2RuntimePrivatePCH.h"

#include "BaseEntityComponent.h"
#include "BaseEntity.h"

// Sets default values for this component's properties
UBaseEntityComponent::UBaseEntityComponent()
{
	// Set this component to be initialized when the game starts, and to not be ticked every frame.
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaseEntityComponent::BeginPlay()
{
	Entity = CastChecked<ABaseEntity>(GetOwner());
}

/**
 * Fires a logic output on the owner entity.
 * Returns the number of entities that succesfully handled the output.
 */
int UBaseEntityComponent::FireOutput(const FName outputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator)
{
	return Entity->FireOutput(outputName, args, caller, activator);
}

void UBaseEntityComponent::OnInputFired_Implementation(const FName inputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator)
{
	// Left for blueprint to override and handle custom inputs
}