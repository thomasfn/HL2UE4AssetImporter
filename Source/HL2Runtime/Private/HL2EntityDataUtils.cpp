// Fill out your copyright notice in the Description page of Project Settings.

#include "HL2EntityDataUtils.h"

#include "HL2Runtime.h"
#include "SourceCoord.h"

FString UHL2EntityDataUtils::GetString(const FHL2EntityData& entityData, FName key) { return entityData.GetString(key); }

bool UHL2EntityDataUtils::TryGetString(const FHL2EntityData& entityData, FName key, FString& out) { return entityData.TryGetString(key, out); }

int UHL2EntityDataUtils::GetInt(const FHL2EntityData& entityData, FName key) { return entityData.GetInt(key); }

bool UHL2EntityDataUtils::TryGetInt(const FHL2EntityData& entityData, FName key, int& out) { return entityData.TryGetInt(key, out); }

bool UHL2EntityDataUtils::GetBool(const FHL2EntityData& entityData, FName key) { return entityData.GetBool(key); }

bool UHL2EntityDataUtils::TryGetBool(const FHL2EntityData& entityData, FName key, bool& out) { return entityData.TryGetBool(key, out); }

float UHL2EntityDataUtils::GetFloat(const FHL2EntityData& entityData, FName key) { return entityData.GetFloat(key); }

bool UHL2EntityDataUtils::TryGetFloat(const FHL2EntityData& entityData, FName key, float& out) { return entityData.TryGetFloat(key, out); }

FVector UHL2EntityDataUtils::GetVector(const FHL2EntityData& entityData, FName key) { return (FVector)entityData.GetVector(key); }

bool UHL2EntityDataUtils::TryGetVector(const FHL2EntityData& entityData, FName key, FVector& out)
{
	FVector3f tmp;
	if (!entityData.TryGetVector(key, tmp)) { return false; }
	out = FVector(tmp);
	return true;
}

FVector4 UHL2EntityDataUtils::GetVector4(const FHL2EntityData& entityData, FName key) { return (FVector4)entityData.GetVector4(key); }

bool UHL2EntityDataUtils::TryGetVector4(const FHL2EntityData& entityData, FName key, FVector4& out)
{
	FVector4f tmp;
	if (!entityData.TryGetVector4(key, tmp)) { return false; }
	out = FVector4(tmp);
	return true;
}

FRotator UHL2EntityDataUtils::GetRotator(const FHL2EntityData& entityData, FName key)
{
	FRotator tmp;
	TryGetRotator(entityData, key, tmp);
	return tmp;
}

bool UHL2EntityDataUtils::TryGetRotator(const FHL2EntityData& entityData, FName key, FRotator& out)
{
	FVector3f value;
	if (!entityData.TryGetVector(key, value)) { return false; }
	out = FRotator(SourceToUnreal.Rotator(FRotator3f::MakeFromEuler(FVector3f(value.Z, -value.X, 180.0f - value.Y))));
	return true;
}

FLinearColor UHL2EntityDataUtils::GetColor(const FHL2EntityData& entityData, FName key)
{
	FLinearColor tmp;
	TryGetColor(entityData, key, tmp);
	return tmp;
}

bool UHL2EntityDataUtils::TryGetColor(const FHL2EntityData& entityData, FName key, FLinearColor& out)
{
	FVector3f value;
	if (!entityData.TryGetVector(key, value)) { return false; }
	out.R = value.X / 255.0f;
	out.G = value.Y / 255.0f;
	out.B = value.Z / 255.0f;
	return true;
}

bool UHL2EntityDataUtils::TryGetColorAndAlpha(const FHL2EntityData& entityData, FName key, FLinearColor& out, float& outAlpha)
{
	FVector4f value;
	if (!entityData.TryGetVector4(key, value)) { return false; }
	out.R = value.X / 255.0f;
	out.G = value.Y / 255.0f;
	out.B = value.Z / 255.0f;
	outAlpha = value.W / 255.0f;
	return true;
}

FVector UHL2EntityDataUtils::ConvertSourcePositionToUnreal(const FVector& position)
{
	return FVector(SourceToUnreal.Position(FVector3f(position)));
}

FQuat UHL2EntityDataUtils::ConvertSourceRotationToUnreal(const FQuat& rotation)
{
	return FQuat(SourceToUnreal.Quat(FQuat4f(rotation)));
}

FTransform UHL2EntityDataUtils::ConvertSourceTransformToUnreal(const FTransform& transform)
{
	return FTransform(SourceToUnreal.Transform(FTransform3f(transform)));
}