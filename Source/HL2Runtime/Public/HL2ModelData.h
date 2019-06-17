#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "HL2ModelData.generated.h"

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FModelSkinMapping
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<int, UMaterialInterface*> MaterialOverrides;

};

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FModelBodygroupMapping
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TSet<int> Sections;

};

USTRUCT(BlueprintType)
struct HL2RUNTIME_API FModelBodygroup
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TSet<int> AllSections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FModelBodygroupMapping> Mappings;

};

UCLASS()
class HL2RUNTIME_API UHL2ModelData : public UAssetUserData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FModelSkinMapping> Skins;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<FName, FModelBodygroup> Bodygroups;

public:

	bool ApplyBodygroupToStaticMesh(UStaticMeshComponent* target, const FName bodygroupName, int index);

	bool ApplySkinToStaticMesh(UStaticMeshComponent* target, int index);

	bool ApplyBodygroupToSkeletalMesh(USkeletalMeshComponent* target, const FName bodygroupName, int index);

	bool ApplySkinToSkeletalMesh(USkeletalMeshComponent* target, int index);

};