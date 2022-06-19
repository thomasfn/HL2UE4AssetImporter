// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Character.h"

#include "HL2BaseCharacter.generated.h"

UCLASS(BlueprintType)
class HL2RUNTIME_API AHL2BaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AHL2BaseCharacter(const FObjectInitializer& ObjectInitializer);

};
