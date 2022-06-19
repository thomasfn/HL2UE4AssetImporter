#pragma once

#include "CoreMinimal.h"
#include "ValveKeyValues.h"

#include "DetailProps.generated.h"

UENUM()
enum class EDetailPropGroupPropType : uint8
{
	Sprite,
	Shape,
	Model
};

USTRUCT()
struct FDetailPropGroupProp
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FName Name = NAME_None;

	UPROPERTY()
	float Amount = 1.0f;

	UPROPERTY()
	bool Upright = false;

	UPROPERTY()
	float MinAngle = 180.0f;

	UPROPERTY()
	float MaxAngle = 180.0f;

	UPROPERTY()
	EDetailPropGroupPropType Type;

public: // EDetailPropGroupPropType::Sprite

	UPROPERTY()
	int SpriteIndex = 0;

	UPROPERTY()
	FVector4f SpriteRect;

	UPROPERTY()
	FVector4f SpriteSize;

	UPROPERTY()
	float SpriteRandomScale;

	UPROPERTY()
	int DetailOrientation;

public: // EDetailPropGroupPropType::Shape

public: // EDetailPropGroupPropType::Model

	UPROPERTY()
	FString Model;

};

USTRUCT()
struct FDetailPropGroup
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FName GroupName = NAME_None;

	UPROPERTY()
	float Alpha = 1.0f;

	UPROPERTY()
	TArray<FDetailPropGroupProp> Props;

};

USTRUCT()
struct FDetailProps
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FName Name;

	UPROPERTY()
	float Density;

	UPROPERTY()
	TArray<FDetailPropGroup> Groups;

public:

	static void ParseAllDetailProps(const UValveDocument* document, TMap<FName, FDetailProps>& outProps);

private:

	static FDetailProps ParseDetailProps(const UValveGroupValue* groupValue);

	static FDetailPropGroup ParseGroup(const UValveGroupValue* groupValue);

	static FDetailPropGroupProp ParseProp(const UValveGroupValue* groupValue);

};