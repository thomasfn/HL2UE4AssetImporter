#include "HL2BaseCharacter.h"

#include "HL2CharacterMovementComponent.h"

AHL2BaseCharacter::AHL2BaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHL2CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{

}
