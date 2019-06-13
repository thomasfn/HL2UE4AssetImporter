// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseEntityComponent.generated.h"

class ABaseEntity;

UCLASS(BlueprintType, abstract, Blueprintable)
class HL2RUNTIME_API UBaseEntityComponent : public UActorComponent
{
	friend class ABaseEntity;

	GENERATED_BODY()

protected:

	/** The entity to whom this component belongs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	ABaseEntity* Entity;


public:

	// Sets default values for this component's properties
	UBaseEntityComponent();

	virtual void BeginPlay() override;

	/**
	 * Fires a logic output on the owner entity.
	 * Returns the number of entities that succesfully handled the output.
	 */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	int FireOutput(const FName outputName, const TArray<FString>& args, ABaseEntity* caller = nullptr, ABaseEntity* activator = nullptr);

protected:

	/**
	 * Published when an input has been fired on the owner entity.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "HL2")
	void OnInputFired(const FName inputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator);

	void OnInputFired_Implementation(const FName inputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator);
		
};
