#pragma once

#include "BSPImporter.h"
#include "Paths.h"

#ifdef WITH_EDITOR
#include "EditorActorFolders.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Builders/EditorBrushBuilder.h"
#include "VBSPBrushBuilder.h"
#endif

DEFINE_LOG_CATEGORY(LogHL2BSPImporter);

FBSPImporter::FBSPImporter()
{

}

FBSPImporter::~FBSPImporter()
{

}

#ifdef WITH_EDITOR

bool FBSPImporter::ImportToCurrentLevel(const FString& fileName)
{
	FString bspDir = FPaths::GetPath(fileName) + "/";
	FString bspFileName = FPaths::GetCleanFilename(fileName);
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing map '%s' to current level"), *bspFileName);
	const auto bspDirConvert = StringCast<ANSICHAR, TCHAR>(*bspDir);
	const auto bspFileNameConvert = StringCast<ANSICHAR, TCHAR>(*bspFileName);
	Valve::BSPFile bspFile;
	if (!bspFile.parse(std::string(bspDirConvert.Get()), std::string(bspFileNameConvert.Get())))
	{
		UE_LOG(LogHL2BSPImporter, Error, TEXT("Failed to parse BSP"));
		return false;
	}
	return ImportToWorld(bspFile, GEditor->GetEditorWorldContext().World());
}

bool FBSPImporter::ImportToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	if (!ImportBrushesToWorld(bspFile, world)) { return false; }
	return true;
}

bool FBSPImporter::ImportBrushesToWorld(const Valve::BSPFile& bspFile, UWorld* world)
{
	const FName brushesFolder = TEXT("HL2Brushes");
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing brushes..."));
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, brushesFolder);
	for (uint32 i = 0, l = bspFile.m_Models.size(); i < l; ++i)
	{
		const Valve::BSP::dmodel_t& model = bspFile.m_Models[i];
		TArray<uint16> brushIndices;
		GatherBrushes(bspFile, model.m_Headnode, brushIndices);
		brushIndices.Sort();

		for (uint16 brushIndex : brushIndices)
		{
			if (i == 0)
			{
				AActor* brushActor = ImportBrush(world, bspFile, brushIndex);
				GEditor->SelectActor(brushActor, true, false, true, false);
				folders.SetSelectedFolderPath(brushesFolder);
			}
			else
			{
				// TODO: Deal with brush entities
			}
		}
	}

	return true;
}

AActor* FBSPImporter::ImportBrush(UWorld* world, const Valve::BSPFile& bspFile, uint16 brushIndex)
{
	const Valve::BSP::dbrush_t& brush = bspFile.m_Brushes[brushIndex];
	ABrush* brushActor = world->SpawnActor<ABrush>(FVector::ZeroVector, FRotator::ZeroRotator);
	brushActor->SetActorLabel(FString::Printf(TEXT("Brush_%d"), brushIndex));
	UVBSPBrushBuilder* brushBuilder = NewObject<UVBSPBrushBuilder>(brushActor);
	brushActor->BrushBuilder = brushBuilder;
	
	for (uint32 i = 0, l = brush.m_Numsides; i < l; ++i)
	{
		const Valve::BSP::dbrushside_t& brushSide = bspFile.m_Brushsides[brush.m_Firstside + i];
		const Valve::BSP::cplane_t& plane = bspFile.m_Planes[brushSide.m_Planenum];

		FVBSPBrushPlane brushPlane;
		brushPlane.Material = nullptr; // TODO: Resolve material
		brushPlane.Plane = ValveToUnrealPlane(plane);
		brushBuilder->Planes.Add(brushPlane);
	}

	brushBuilder->Build(world, brushActor);

	return brushActor;
}

void FBSPImporter::GatherBrushes(const Valve::BSPFile& bspFile, uint32 nodeIndex, TArray<uint16>& out)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1-node.m_Children[0]];
		for (uint32 i = 0; i < leaf.m_Numleafbrushes; ++i)
		{
			out.AddUnique(bspFile.m_Leafbrushes[leaf.m_Firstleafbrush + i]);
		}
	}
	else
	{
		GatherBrushes(bspFile, (uint32)node.m_Children[0], out);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		for (uint32 i = 0; i < leaf.m_Numleafbrushes; ++i)
		{
			out.AddUnique(bspFile.m_Leafbrushes[leaf.m_Firstleafbrush + i]);
		}
	}
	else
	{
		GatherBrushes(bspFile, (uint32)node.m_Children[1], out);
	}
}

FPlane FBSPImporter::ValveToUnrealPlane(const Valve::BSP::cplane_t& plane)
{
	return FPlane(plane.m_Normal(0, 0), plane.m_Normal(0, 1), plane.m_Normal(0, 2), plane.m_Distance);
}

#endif