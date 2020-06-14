#include "VBSPInfo.h"

#include "BaseEntity.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

/** Gets the leaf that contains the position, or -1 if the position is outside the BSP tree. */
int AVBSPInfo::FindLeaf(const FVector& pos) const
{
	if (Nodes.Num() == 0) { return -1; }
	int curNodeID = 0;
	FVector flippedPos = pos * FVector(1.0f, -1.0f, 1.0f);
	while (curNodeID >= 0)
	{
		check(curNodeID < Nodes.Num());
		const FVBSPNode& node = Nodes[curNodeID];
		const bool isFront = node.Plane.PlaneDot(pos) >= 0.0f;
		curNodeID = isFront ? node.Right : node.Left;
	}
	return NodeIDToLeafID(curNodeID);
}

/** Gets the cluster that contains the position, or -1 if the position is not inside a cluster. */
int AVBSPInfo::FindCluster(const FVector& pos) const
{
	const int leafID = FindLeaf(pos);
	if (leafID < 0) { return -1; }
	check(leafID < Leaves.Num());
	const FVBSPLeaf& leaf = Leaves[leafID];
	return leaf.Cluster;
}

/** Finds all clusters that are reachable from the specified one. */
void AVBSPInfo::FindReachableClusters(const int baseCluster, TSet<int>& out) const
{
	if (baseCluster < 0) { return; }
	TArray<int> clusterStack;
	clusterStack.Push(baseCluster);
	out.Add(baseCluster);
	while (clusterStack.Num() > 0)
	{
		const int clusterID = clusterStack.Pop();
		check(clusterID >= 0 && clusterID < Clusters.Num());
		const FVBSPCluster& cluster = Clusters[clusterID];
		for (const int otherClusterID : cluster.VisibleClusters)
		{
			bool alreadyInSet;
			out.Add(otherClusterID, &alreadyInSet);
			if (!alreadyInSet)
			{
				clusterStack.Push(otherClusterID);
			}
		}
	}
}

/** Finds all entities that are contained within the cluster. Only checks origin point of entity, not entire bounds. */
void AVBSPInfo::FindEntitiesInCluster(const int clusterIndex, TSet<ABaseEntity*>& out) const
{
	// Iterate all entities
	for (TActorIterator<ABaseEntity> it(GetWorld()); it; ++it)
	{
		ABaseEntity* entity = *it;
		if (FindCluster(entity->GetRootComponent()->GetComponentLocation()) == clusterIndex)
		{
			out.Add(entity);
		}
	}
}

/** Finds all entities that are contained within one of the clusters. Only checks origin point of entity, not entire bounds. */
void AVBSPInfo::FindEntitiesInClusters(const TSet<int>& clusterIndices, TSet<ABaseEntity*>& out) const
{
	// Iterate all entities
	for (TActorIterator<ABaseEntity> it(GetWorld()); it; ++it)
	{
		ABaseEntity* entity = *it;
		if (clusterIndices.Contains(FindCluster(entity->GetRootComponent()->GetComponentLocation())))
		{
			out.Add(entity);
		}
	}
}

int AVBSPInfo::NodeIDToLeafID(int nodeID) const
{
	if (nodeID >= 0) { return -1; }
	return -(nodeID + 1);
}