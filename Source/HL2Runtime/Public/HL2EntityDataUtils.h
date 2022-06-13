// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HL2EntityData.h"

#include "HL2EntityDataUtils.generated.h"

/**
 * 
 */
UCLASS()
class HL2RUNTIME_API UHL2EntityDataUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FString GetString(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetString(const FHL2EntityData& entityData, FName key, FString& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static int GetInt(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetInt(const FHL2EntityData& entityData, FName key, int& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool GetBool(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetBool(const FHL2EntityData& entityData, FName key, bool& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static float GetFloat(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetFloat(const FHL2EntityData& entityData, FName key, float& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FVector GetVector(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetVector(const FHL2EntityData& entityData, FName key, FVector& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FVector4 GetVector4(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetVector4(const FHL2EntityData& entityData, FName key, FVector4& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FRotator GetRotator(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetRotator(const FHL2EntityData& entityData, FName key, FRotator& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FLinearColor GetColor(const FHL2EntityData& entityData, FName key);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetColor(const FHL2EntityData& entityData, FName key, FLinearColor& out);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static bool TryGetColorAndAlpha(const FHL2EntityData& entityData, FName key, FLinearColor& out, float& outAlpha);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FVector ConvertSourcePositionToUnreal(const FVector& position);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FQuat ConvertSourceRotationToUnreal(const FQuat& rotation);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HL2")
	static FTransform ConvertSourceTransformToUnreal(const FTransform& transform);
	
};
