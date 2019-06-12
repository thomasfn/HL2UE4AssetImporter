// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HL2EntityData.h"

#include "BaseEntity.generated.h"

UCLASS()
class HL2RUNTIME_API ABaseEntity : public AActor
{
	GENERATED_BODY()
	
public:

	/** The raw entity data straight from the vbsp entities lump. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	FHL2EntityData EntityData;

	/** The world model, if any, for this entity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	UStaticMesh* WorldModel;

	/* Initialises the entity from entity data. This is called once in the editor context when the entity is imported from the vbsp. */
	//UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "HL2")
	//void InitialiseEntity();

	//void InitialiseEntity_Implementation();


};
