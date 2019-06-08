// Fill out your copyright notice in the Description page of Project Settings.

#include "HL2Runtime.h"
#include "HL2EntityDataUtils.h"

FString UHL2EntityDataUtils::GetString(const FHL2EntityData& entityData, FName key) { return entityData.GetString(key); }

bool UHL2EntityDataUtils::TryGetString(const FHL2EntityData& entityData, FName key, FString& out) { return entityData.TryGetString(key, out); }

int UHL2EntityDataUtils::GetInt(const FHL2EntityData& entityData, FName key) { return entityData.GetInt(key); }

bool UHL2EntityDataUtils::TryGetInt(const FHL2EntityData& entityData, FName key, int& out) { return entityData.TryGetInt(key, out); }

bool UHL2EntityDataUtils::GetBool(const FHL2EntityData& entityData, FName key) { return entityData.GetBool(key); }

bool UHL2EntityDataUtils::TryGetBool(const FHL2EntityData& entityData, FName key, bool& out) { return entityData.TryGetBool(key, out); }

float UHL2EntityDataUtils::GetFloat(const FHL2EntityData& entityData, FName key) { return entityData.GetFloat(key); }

bool UHL2EntityDataUtils::TryGetFloat(const FHL2EntityData& entityData, FName key, float& out) { return entityData.TryGetFloat(key, out); }

FVector UHL2EntityDataUtils::GetVector(const FHL2EntityData& entityData, FName key) { return entityData.GetVector(key); }

bool UHL2EntityDataUtils::TryGetVector(const FHL2EntityData& entityData, FName key, FVector& out) { return entityData.TryGetVector(key, out); }

FRotator UHL2EntityDataUtils::GetRotator(const FHL2EntityData& entityData, FName key) { return entityData.GetRotator(key); }

bool UHL2EntityDataUtils::TryGetRotator(const FHL2EntityData& entityData, FName key, FRotator& out) { return entityData.TryGetRotator(key, out); }