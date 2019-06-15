// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "VBSPInfo.generated.h"

class ABaseEntity;
class AStaticMeshActor;

USTRUCT(BlueprintType)
struct FVBSPNode
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FPlane Plane;

};

USTRUCT(BlueprintType)
struct FVBSPLeaf
{
	GENERATED_BODY()

public:

	/** All cells that intersect this leaf. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<AStaticMeshActor*> Cells;

	/** The cluster to which this leaf belongs, or -1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Cluster;

	/** Whether this leaf is considered solid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Solid;

};

USTRUCT(BlueprintType)
struct FVBSPCluster
{
	GENERATED_BODY()

public:

	/** All cells that intersect this cluster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<AStaticMeshActor*> Cells;

	/** All clusters that can be seen from this cluster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<int> VisibleClusters;

	/** All leaves contained within this cluster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<int> Leaves;

};

UCLASS()
class HL2RUNTIME_API AVBSPInfo : public AActor
{
	GENERATED_BODY()
	
public:

	/** The VBSP nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FVBSPNode> Nodes;

	/** The VBSP leaves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FVBSPLeaf> Leaves;

	/** The VBSP clusters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FVBSPCluster> Clusters;

public:

	/** Gets the leaf that contains the position, or -1 if the position is outside the BSP tree. */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	int FindLeaf(const FVector& pos) const;

	/** Gets the cluster that contains the position, or -1 if the position is not inside a cluster. */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	int FindCluster(const FVector& pos) const;

	/** Finds all clusters that are reachable from the specified one. */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	void FindReachableClusters(const int baseCluster, TSet<int>& out) const;

	/** Finds all entities that are contained within the cluster. Only checks origin point of entity, not entire bounds. */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	void FindEntitiesInCluster(const int clusterIndex, TSet<ABaseEntity*>& out) const;

	/** Finds all entities that are contained within one of the clusters. Only checks origin point of entity, not entire bounds. */
	UFUNCTION(BlueprintCallable, Category = "HL2")
	void FindEntitiesInClusters(const TSet<int>& clusterIndices, TSet<ABaseEntity*>& out) const;

protected:

	UFUNCTION()
	int NodeIDToLeafID(int nodeID) const;

};
