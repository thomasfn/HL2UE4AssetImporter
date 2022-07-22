#pragma once

#include "BSPImporter.h"
#include "IHL2Runtime.h"
#include "Misc/Paths.h"
#include "EditorActorFolders.h"
#include "Engine/World.h"
#include "Engine/Brush.h"
#include "Builders/CubeBuilder.h"
#include "Engine/Polys.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "Model.h"
#include "Lightmass/LightmassImportanceVolume.h"
#include "Builders/EditorBrushBuilder.h"
#include "Components/BrushComponent.h"
#include "BSPBrushUtils.h"
#include "MeshAttributes.h"
#include "StaticMeshAttributes.h"
#include "Internationalization/Regex.h"
#include "MeshAttributes.h"
#include "MeshUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Misc/ScopedSlowTask.h"
#include "EngineUtils.h"
#include "MeshDescriptionOperations.h"
#include "MeshUtilitiesCommon.h"
#include "OverlappingCorners.h"
#include "SourceCoord.h"
#include "IHL2Editor.h"
#include "EntityEmitter.h"

DEFINE_LOG_CATEGORY(LogHL2BSPImporter);

#define LOCTEXT_NAMESPACE "HL2Importer"

FBSPImporter::FBSPImporter(const FString& fileName) :
	bspFileName(fileName),
	bspLoaded(false),
	mapName(FPaths::GetCleanFilename(fileName)),
	world(nullptr),
	vbspInfo(nullptr)
{ }

bool FBSPImporter::Load()
{
	FString path = FPaths::GetPath(bspFileName) + TEXT("/");
	FString fileName = FPaths::GetCleanFilename(bspFileName);
	mapName = FPaths::GetBaseFilename(bspFileName);
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Loading map '%s'..."), *mapName);
	const auto pathConvert = StringCast<ANSICHAR>(*path, path.Len() + 1);
	const auto fileNameConvert = StringCast<ANSICHAR>(*fileName, fileName.Len() + 1);
	if (!bspFile.parse(std::string(pathConvert.Get()), std::string(fileNameConvert.Get())))
	{
		UE_LOG(LogHL2BSPImporter, Error, TEXT("Failed to parse BSP"));
		return false;
	}
	return true;
}

bool FBSPImporter::ImportToCurrentLevel()
{
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing map '%s' to current level"), *bspFileName);
	return ImportAllToWorld(GEditor->GetEditorWorldContext().World());
}

bool FBSPImporter::ImportAllToWorld(UWorld* targetWorld)
{
	FScopedSlowTask loopProgress(2, LOCTEXT("MapImporting", "Importing map..."));
	loopProgress.MakeDialog();

	loopProgress.EnterProgressFrame(1.0f);
	if (!ImportGeometryToWorld(targetWorld)) { return false; }

	loopProgress.EnterProgressFrame(1.0f);
	if (!ImportEntitiesToWorld(targetWorld)) { return false; }

	//loopProgress.EnterProgressFrame(1.0f);
	//if (!ImportBrushesToWorld(targetWorld)) { return false; }

	return true;
}

bool FBSPImporter::ImportGeometryToWorld(UWorld* targetWorld)
{
	world = targetWorld;

	const FFolder geometryFolder(TEXT("HL2Geometry"));
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing geometry..."));
	FActorFolders& folders = FActorFolders::Get();
	folders.CreateFolder(*world, geometryFolder);

	const Valve::BSP::dmodel_t& bspWorldModel = bspFile.m_Models[0];

	TArray<AStaticMeshActor*> staticMeshes;
	RenderModelToActors(staticMeshes, 0);
	GEditor->SelectNone(false, true, false);
	for (AStaticMeshActor* actor : staticMeshes)
	{
		GEditor->SelectActor(actor, true, false, true, false);
	}
	folders.SetSelectedFolderPath(geometryFolder);
	GEditor->SelectNone(false, true, false);

	const FBox3f bspBounds = GetModelBounds(bspWorldModel, true);
	ALightmassImportanceVolume* lightmassImportanceVolume = world->SpawnActor<ALightmassImportanceVolume>();
	lightmassImportanceVolume->Brush = NewObject<UModel>(lightmassImportanceVolume, NAME_None, RF_Transactional);
	lightmassImportanceVolume->Brush->Initialize(nullptr, true);
	lightmassImportanceVolume->Brush->Polys = NewObject<UPolys>(lightmassImportanceVolume->Brush, NAME_None, RF_Transactional);
	lightmassImportanceVolume->GetBrushComponent()->Brush = lightmassImportanceVolume->Brush;
	lightmassImportanceVolume->SetActorLocation(FVector(bspBounds.GetCenter()));
	UCubeBuilder* brushBuilder = NewObject<UCubeBuilder>(lightmassImportanceVolume);
	const FVector3f extent = bspBounds.GetExtent();
	brushBuilder->X = extent.X;
	brushBuilder->Y = extent.Y;
	brushBuilder->Z = extent.Z;
	brushBuilder->Build(world, lightmassImportanceVolume);

	return true;
}

bool FBSPImporter::ImportEntitiesToWorld(UWorld* targetWorld)
{
	world = targetWorld;

	const FFolder entitiesFolder(TEXT("HL2Entities"));
	UE_LOG(LogHL2BSPImporter, Log, TEXT("Importing entities..."));

	// Read entities lump
	const auto entityStrRaw = StringCast<TCHAR>(&bspFile.m_Entities[0], bspFile.m_Entities.size());
	FString entityStr(entityStrRaw.Get());

	// Parse into entity data
	TArray<FHL2EntityData> entityDatas;
	if (!FEntityParser::ParseEntities(entityStr, entityDatas)) { return false; }

	// Parse static props
	const static FName fnStaticProp(TEXT("prop_static"));
	const static FName fnSolidity(TEXT("solidity"));
	const static FName fnModel(TEXT("model"));
	const static FName fnSkin(TEXT("skin"));
	const static FName fnAngles(TEXT("angles"));
	// TODO: Find a way to not have all these dumb loops
	for (const Valve::BSP::StaticProp_v4_t staticProp : bspFile.m_Staticprops_v4)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector3f(staticProp.m_Origin(0, 0), staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues.Add(fnSolidity, FString::FromInt((int)staticProp.m_Solid));
		const char *modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto &modelRaw = StringCast<TCHAR, 128>(modelStr);
		entityData.KeyValues.Add(fnModel, FString(modelRaw.Length(), modelRaw.Get()));
		entityData.KeyValues.Add(fnAngles, FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2)));
		entityData.KeyValues.Add(fnSkin, FString::FromInt(staticProp.m_Skin));
		entityDatas.Add(entityData);
	}
	for (const Valve::BSP::StaticProp_v5_t staticProp : bspFile.m_Staticprops_v5)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector3f(staticProp.m_Origin(0, 0), staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues.Add(fnSolidity, FString::FromInt((int)staticProp.m_Solid));
		const char *modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto &modelRaw = StringCast<TCHAR, 128>(modelStr);
		entityData.KeyValues.Add(fnModel, FString(modelRaw.Length(), modelRaw.Get()));
		entityData.KeyValues.Add(fnAngles, FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2)));
		entityData.KeyValues.Add(fnSkin, FString::FromInt(staticProp.m_Skin));
		entityDatas.Add(entityData);
	}
	for (const Valve::BSP::StaticProp_v6_t staticProp : bspFile.m_Staticprops_v6)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector3f(staticProp.m_Origin(0, 0), staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues.Add(fnSolidity, FString::FromInt((int)staticProp.m_Solid));
		const char* modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto& modelRaw = StringCast<TCHAR, 128>(modelStr);
		entityData.KeyValues.Add(fnModel, FString(modelRaw.Length(), modelRaw.Get()));
		entityData.KeyValues.Add(fnAngles, FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2)));
		entityData.KeyValues.Add(fnSkin, FString::FromInt(staticProp.m_Skin));
		entityDatas.Add(entityData);
	}
	for (const Valve::BSP::StaticProp_v10_t staticProp : bspFile.m_Staticprops_v10)
	{
		FHL2EntityData entityData;
		entityData.Classname = fnStaticProp;
		entityData.Origin = FVector3f(staticProp.m_Origin(0, 0), staticProp.m_Origin(0, 1), staticProp.m_Origin(0, 2));
		entityData.KeyValues.Add(fnSolidity, FString::FromInt((int)staticProp.m_Solid));
		const char* modelStr = bspFile.m_StaticpropStringTable[staticProp.m_PropType].m_Str;
		const auto& modelRaw = StringCast<TCHAR, 128>(modelStr);
		entityData.KeyValues.Add(fnModel, FString(modelRaw.Length(), modelRaw.Get()));
		entityData.KeyValues.Add(fnAngles, FString::Printf(TEXT("%f %f %f"), staticProp.m_Angles(0, 0), staticProp.m_Angles(0, 1), staticProp.m_Angles(0, 2)));
		entityData.KeyValues.Add(fnSkin, FString::FromInt(staticProp.m_Skin));
		entityDatas.Add(entityData);
	}

	// Parse cubemaps
	const FHL2EditorBSPConfig& bspConfig = IHL2Editor::Get().GetConfig().BSP;
	if (bspConfig.EmitReflectionCaptures)
	{
		const static FName fnCubemap(TEXT("env_cubemap"));
		const static FName fnSize(TEXT("size"));
		for (const Valve::BSP::dcubemapsample_t cubemap : bspFile.m_Cubemaps)
		{
			FHL2EntityData entityData;
			entityData.Classname = fnCubemap;
			entityData.Origin = FVector3f(cubemap.m_Origin[0], cubemap.m_Origin[1], cubemap.m_Origin[2]);
			entityData.KeyValues.Add(fnSize, FString::FromInt(cubemap.m_Size));
			entityDatas.Add(entityData);
		}
	}

	// Generate models
	TArray<UStaticMesh*> bspModels;
	bspModels.Reserve(bspFile.m_Models.size());
	bspModels.Add(nullptr); // brushes aren't going to reference the worldmodel
	for (int i = 1; i < bspFile.m_Models.size(); ++i)
	{
		const Valve::BSP::dmodel_t& bspModel = bspFile.m_Models[i];
		TArray<uint16> faces;
		GatherFaces(bspModel.m_Headnode, faces);
		FMeshDescription meshDesc;
		FStaticMeshAttributes staticMeshAttr(meshDesc);
		staticMeshAttr.Register();
		staticMeshAttr.RegisterTriangleNormalAndTangentAttributes();
		RenderFacesToMesh(faces, meshDesc, false);
		FStaticMeshOperations::ComputeTangentsAndNormals(meshDesc, EComputeNTBsFlags::Normals & EComputeNTBsFlags::Tangents);
		meshDesc.TriangulateMesh();
		const int lightmapResolution = 128;
		bspModels.Add(RenderMeshToStaticMesh(meshDesc, FString::Printf(TEXT("Models/Model_%d"), i), lightmapResolution));
	}

	// Convert into actors
	FScopedSlowTask progress(entityDatas.Num(), LOCTEXT("MapEntitiesImporting", "Importing map entities..."));
	FEntityEmitter emitter(targetWorld, bspFile, bspModels, vbspInfo);
	emitter.GenerateActors(entityDatas, &progress);

	return true;
}

void FBSPImporter::RenderModelToActors(TArray<AStaticMeshActor*>& out, uint32 modelIndex)
{
	const FHL2EditorBSPConfig& bspConfig = IHL2Editor::Get().GetConfig().BSP;

	const Valve::BSP::dmodel_t& bspModel = bspFile.m_Models[modelIndex];

	// Determine cell mins and maxs
	const Valve::BSP::snode_t& bspNode = bspFile.m_Nodes[bspModel.m_Headnode];
	const FBox3f bspBounds = GetNodeBounds(bspNode, true);
	const int cellMinX = FMath::FloorToInt(bspBounds.Min.X / bspConfig.CellSize);
	const int cellMaxX = FMath::CeilToInt(bspBounds.Max.X / bspConfig.CellSize);
	const int cellMinY = FMath::FloorToInt(bspBounds.Min.Y / bspConfig.CellSize);
	const int cellMaxY = FMath::CeilToInt(bspBounds.Max.Y / bspConfig.CellSize);

	FScopedSlowTask progress((cellMaxX - cellMinX + 1) * (cellMaxY - cellMinY + 1) + 33, LOCTEXT("MapGeometryImporting", "Importing map geometry..."));
	progress.MakeDialog();

	// Render out VBSPInfo
	//progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_VBSPINFO", "Generating VBSPInfo..."));
	//RenderTreeToVBSPInfo(bspModel.m_Headnode);

	// Gather all faces and displacements from tree
	progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_GATHER", "Gathering faces and displacements..."));
	TArray<uint16> faces;
	faces.Reserve(bspModel.m_Numfaces);
	for (int i = 0; i < bspModel.m_Numfaces; ++i)
	{
		faces.Add(bspModel.m_Firstface + i);
	}
	TArray<uint16> brushes;
	GatherBrushes(bspModel.m_Headnode, brushes);
	TArray<uint16> displacements;
	GatherDisplacements(faces, displacements);

	{
		// Render whole tree to a single mesh
		progress.EnterProgressFrame(10.0f, LOCTEXT("MapGeometryImporting_GENERATE", "Generating map geometry..."));
		FMeshDescription meshDesc;
		FStaticMeshAttributes staticMeshAttr(meshDesc);
		staticMeshAttr.Register();
		staticMeshAttr.RegisterTriangleNormalAndTangentAttributes();
		//RenderFacesToMesh(faces, meshDesc, false);
		RenderBrushesToMesh(brushes, meshDesc);
		//RenderDisplacementsToMesh(displacements, meshDesc);
		FStaticMeshOperations::ComputeTangentsAndNormals(meshDesc, EComputeNTBsFlags::Normals & EComputeNTBsFlags::Tangents);
		//FMeshUtils::Clean(meshDesc, FMeshCleanSettings::All);

		if (bspConfig.UseCells)
		{
			// Split mesh into cells in parallel
			progress.EnterProgressFrame(10.0f, LOCTEXT("MapGeometryImporting_CELL", "Splitting cells..."));
			TArray<FMeshDescription> cellMeshes;
			TArray<int> lightmapResolutions;
			const int cellCountX = (cellMaxX - cellMinX) + 1;
			const int cellCountY = (cellMaxY - cellMinY) + 1;
			const int cellCount = cellCountX * cellCountY;
			cellMeshes.AddDefaulted(cellCount);
			lightmapResolutions.AddDefaulted(cellCount);
			auto iterFunc = [&](int cellIndex)
			{
				int cellX = cellMinX + cellIndex / cellCountX;
				int cellY = cellMinY + cellIndex % cellCountX;
				FMeshDescription& cellMeshDesc = cellMeshes[cellIndex];
				cellMeshDesc = meshDesc;

				// Establish bounding planes for cell
				TArray<FPlane4f> boundingPlanes;
				boundingPlanes.Add(FPlane4f(FVector3f(cellX * bspConfig.CellSize), FVector3f::ForwardVector));
				boundingPlanes.Add(FPlane4f(FVector3f((cellX + 1) * bspConfig.CellSize), FVector3f::BackwardVector));
				boundingPlanes.Add(FPlane4f(FVector3f(cellY * bspConfig.CellSize), FVector3f::RightVector));
				boundingPlanes.Add(FPlane4f(FVector3f((cellY + 1) * bspConfig.CellSize), FVector3f::LeftVector));

				// Clip the mesh by the planes
				FMeshUtils::Clip(cellMeshDesc, boundingPlanes);

				// Check if it has anything
				if (cellMeshDesc.Triangles().Num() > 0)
				{
					// Evaluate mesh surface area and calculate an appropiate lightmap resolution
					const float totalSurfaceArea = FMeshUtils::FindSurfaceArea(cellMeshDesc);
					constexpr float luxelsPerSquareUnit = 1.0f / 8.0f;
					const int lightmapResolution = FMath::Pow(2.0f, FMath::RoundToFloat(FMath::Log2((int)FMath::Sqrt(totalSurfaceArea * luxelsPerSquareUnit))));
					lightmapResolutions[cellIndex] = lightmapResolution;

					// Generate lightmap UVs
					if (bspConfig.GenerateLightmapCoords)
					{
						FMeshUtils::GenerateLightmapCoords(cellMeshDesc, lightmapResolution);
					}
				}
			};
			if (bspConfig.ParallelizeCellSplitting)
			{
				ParallelFor(cellCount, iterFunc);
			}
			else
			{
				for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex)
				{
					iterFunc(cellIndex);
				}
			}

			// Generate static mesh actors for cells
			for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex)
			{
				int cellX = cellMinX + cellIndex / cellCountX;
				int cellY = cellMinY + cellIndex % cellCountX;
				const FMeshDescription& cellMeshDesc = cellMeshes[cellIndex];
				progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_CELL", "Splitting cells..."));

				// Check if it has anything
				if (cellMeshDesc.Triangles().Num() > 0)
				{
					// Create a static mesh for it
					AStaticMeshActor* staticMeshActor = RenderMeshToActor(cellMeshDesc, FString::Printf(TEXT("Cells/Cell_%d"), cellIndex), lightmapResolutions[cellIndex]);
					staticMeshActor->SetActorLabel(FString::Printf(TEXT("Cell_%d_%d"), cellX, cellY));
					out.Add(staticMeshActor);

					// TODO: Insert to VBSPInfo
				}
			}
		}
		else
		{
			// Clean
			//FMeshCleanSettings cleanSettings = FMeshCleanSettings::None;
			//cleanSettings.Retriangulate = true;
			FMeshUtils::Clean(meshDesc);

			// TODO: Evaluate mesh surface area and calculate an appropiate lightmap resolution
			// Evaluate mesh surface area and calculate an appropiate lightmap resolution
			const float totalSurfaceArea = FMeshUtils::FindSurfaceArea(meshDesc);
			constexpr float luxelsPerSquareUnit = 1.0f / 16.0f;
			const int lightmapResolution = FMath::Pow(2.0f, FMath::RoundToFloat(FMath::Log2((int)FMath::Sqrt(totalSurfaceArea * luxelsPerSquareUnit))));

			// Generate lightmap UVs
			if (bspConfig.GenerateLightmapCoords)
			{
				FMeshUtils::GenerateLightmapCoords(meshDesc, lightmapResolution);
			}

			// Create a static mesh for it
			AStaticMeshActor* staticMeshActor = RenderMeshToActor(meshDesc, TEXT("WorldGeometry"), lightmapResolution);
			staticMeshActor->SetActorLabel(TEXT("WorldGeometry"));
			out.Add(staticMeshActor);
		}
	}

	{
		progress.EnterProgressFrame(10.0f, LOCTEXT("MapGeometryImporting_DISPLACEMENTS", "Generating displacement geometry..."));
		
		if (bspConfig.UseDisplacementCells)
		{
			// Render all displacements to a single static mesh, and split into cells
			const int dcellMinX = FMath::FloorToInt(bspBounds.Min.X / bspConfig.DisplacementCellSize);
			const int dcellMaxX = FMath::CeilToInt(bspBounds.Max.X / bspConfig.DisplacementCellSize);
			const int dcellMinY = FMath::FloorToInt(bspBounds.Min.Y / bspConfig.DisplacementCellSize);
			const int dcellMaxY = FMath::CeilToInt(bspBounds.Max.Y / bspConfig.DisplacementCellSize);

			FMeshDescription meshDesc;
			{
				FStaticMeshAttributes staticMeshAttr(meshDesc);
				staticMeshAttr.Register();
				staticMeshAttr.RegisterTriangleNormalAndTangentAttributes();
			}
			RenderDisplacementsToMesh(displacements, meshDesc);

			// Iterate each cell
			int cellIndex = 0;
			for (int cellX = cellMinX; cellX <= cellMaxX; ++cellX)
			{
				for (int cellY = cellMinY; cellY <= cellMaxY; ++cellY)
				{
					// Establish bounding planes for cell
					TArray<FPlane4f> boundingPlanes;
					boundingPlanes.Add(FPlane4f(FVector3f(cellX * bspConfig.CellSize), FVector3f::ForwardVector));
					boundingPlanes.Add(FPlane4f(FVector3f((cellX + 1) * bspConfig.CellSize), FVector3f::BackwardVector));
					boundingPlanes.Add(FPlane4f(FVector3f(cellY * bspConfig.CellSize), FVector3f::RightVector));
					boundingPlanes.Add(FPlane4f(FVector3f((cellY + 1) * bspConfig.CellSize), FVector3f::LeftVector));

					// Clip the mesh by the planes into a new one
					FMeshDescription cellMeshDesc = meshDesc;
					FMeshUtils::Clip(cellMeshDesc, boundingPlanes);

					FStaticMeshOperations::ComputeTangentsAndNormals(cellMeshDesc, EComputeNTBsFlags::Normals & EComputeNTBsFlags::Tangents);

					// Check if it has anything
					if (cellMeshDesc.Polygons().Num() > 0)
					{
						const float totalSurfaceArea = FMeshUtils::FindSurfaceArea(cellMeshDesc);
						constexpr float luxelsPerSquareUnit = 1.0f / 16.0f;
						const int lightmapResolution = (int)FMath::Max(4.0f, FMath::Pow(2.0f, FMath::RoundToFloat(FMath::Log2((int)FMath::Sqrt(totalSurfaceArea * luxelsPerSquareUnit)))));

						if (bspConfig.GenerateLightmapCoords)
						{
							FStaticMeshAttributes staticMeshAttr(cellMeshDesc);
							staticMeshAttr.GetVertexInstanceUVs().SetNumChannels(2);
							FOverlappingCorners overlappingCorners;
							FStaticMeshOperations::FindOverlappingCorners(overlappingCorners, cellMeshDesc, 1.0f / 512.0f);
							FStaticMeshOperations::CreateLightMapUVLayout(cellMeshDesc, 0, 1, lightmapResolution, ELightmapUVVersion::Latest, overlappingCorners);
						}

						// Create a static mesh for it
						AStaticMeshActor* staticMeshActor = RenderMeshToActor(cellMeshDesc, FString::Printf(TEXT("Cells/DisplacementCell_%d"), cellIndex++), lightmapResolution);
						staticMeshActor->SetActorLabel(FString::Printf(TEXT("DisplacementCell_%d_%d"), cellX, cellY));
						out.Add(staticMeshActor);

						// TODO: Insert to VBSPInfo
					}
				}
			}
		}
		else
		{
			// Render all displacements to individual static meshes
			int displacementIndex = 0;
			for (const uint16 displacementID : displacements)
			{
				FMeshDescription meshDesc;
				FStaticMeshAttributes staticMeshAttr(meshDesc);
				staticMeshAttr.Register();
				staticMeshAttr.RegisterTriangleNormalAndTangentAttributes();

				TArray<uint16> tmp = { displacementID };
				RenderDisplacementsToMesh(tmp, meshDesc);

				FStaticMeshOperations::ComputeTangentsAndNormals(meshDesc, EComputeNTBsFlags::Normals & EComputeNTBsFlags::Tangents);

				const float totalSurfaceArea = FMeshUtils::FindSurfaceArea(meshDesc);
				constexpr float luxelsPerSquareUnit = 1.0f / 16.0f;
				const int lightmapResolution = FMath::Pow(2.0f, FMath::RoundToFloat(FMath::Log2((int)FMath::Sqrt(totalSurfaceArea * luxelsPerSquareUnit))));

				if (bspConfig.GenerateLightmapCoords)
				{
					staticMeshAttr.GetVertexInstanceUVs().SetNumChannels(2);
					FOverlappingCorners overlappingCorners;
					FStaticMeshOperations::FindOverlappingCorners(overlappingCorners, meshDesc, 1.0f / 512.0f);
					FStaticMeshOperations::CreateLightMapUVLayout(meshDesc, 0, 1, lightmapResolution, ELightmapUVVersion::Latest, overlappingCorners);
				}

				// Create a static mesh for it
				AStaticMeshActor* staticMeshActor = RenderMeshToActor(meshDesc, FString::Printf(TEXT("Displacements/Displacement_%d"), displacementIndex++), lightmapResolution);
				staticMeshActor->SetActorLabel(FString::Printf(TEXT("Displacement_%d"), displacementID));
				out.Add(staticMeshActor);
			}
		}
	}

	{
		progress.EnterProgressFrame(1.0f, LOCTEXT("MapGeometryImporting_SKYBOX", "Generating skybox geometry..."));

		// Render skybox to a single mesh
		FMeshDescription meshDesc;
		FStaticMeshAttributes staticMeshAttr(meshDesc);
		staticMeshAttr.Register();
		staticMeshAttr.RegisterTriangleNormalAndTangentAttributes();
		RenderFacesToMesh(faces, meshDesc, true);
		FStaticMeshOperations::ComputeTangentsAndNormals(meshDesc, EComputeNTBsFlags::Normals & EComputeNTBsFlags::Tangents);
		meshDesc.TriangulateMesh();

		// Create actor for it
		AStaticMeshActor* staticMeshActor = RenderMeshToActor(meshDesc, TEXT("SkyboxMesh"), 16);
		staticMeshActor->SetActorLabel(TEXT("Skybox"));
		staticMeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		staticMeshActor->GetStaticMeshComponent()->CastShadow = false;
		staticMeshActor->PostEditChange();
		staticMeshActor->MarkPackageDirty();
		out.Add(staticMeshActor);
	}
}

UStaticMesh* FBSPImporter::RenderMeshToStaticMesh(const FMeshDescription& meshDesc, const FString& assetName, int lightmapResolution)
{
	FString packageName = TEXT("/Game/hl2/maps") / mapName / assetName;
	UPackage* package = CreatePackage(*packageName);

	UStaticMesh* staticMesh = NewObject<UStaticMesh>(package, FName(*(FPaths::GetBaseFilename(assetName))), RF_Public | RF_Standalone);
	FStaticMeshSourceModel& staticMeshSourceModel = staticMesh->AddSourceModel();
	FMeshBuildSettings& settings = staticMeshSourceModel.BuildSettings;
	settings.bRecomputeNormals = false;
	settings.bRecomputeTangents = false;
	settings.bGenerateLightmapUVs = false;
	settings.SrcLightmapIndex = 0;
	settings.DstLightmapIndex = 1;
	settings.bRemoveDegenerates = false;
	settings.bUseFullPrecisionUVs = true;
	settings.MinLightmapResolution = lightmapResolution;
	settings.MaxLumenMeshCards = 32;
	staticMesh->SetLightMapResolution(lightmapResolution);
	FMeshDescription* worldModelMesh = staticMesh->CreateMeshDescription(0);
	*worldModelMesh = meshDesc;
	const auto& importedMaterialSlotNameAttr = worldModelMesh->PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);
	TArray<FStaticMaterial> staticMaterials;
	TMap<FPolygonGroupID, int> polygonGroupToMaterialIndex;
	for (const FPolygonGroupID& polyGroupID : worldModelMesh->PolygonGroups().GetElementIDs())
	{
		const FName materialName = importedMaterialSlotNameAttr[polyGroupID];
		const FStaticMaterial staticMaterial(
			Cast<UMaterialInterface>(IHL2Runtime::Get().TryResolveHL2Material(materialName.ToString())),
			materialName,
			materialName
		);
		polygonGroupToMaterialIndex.Add(polyGroupID, staticMaterials.Add(staticMaterial));
	}
	staticMesh->SetStaticMaterials(staticMaterials);
	for (const FPolygonGroupID& polyGroupID : worldModelMesh->PolygonGroups().GetElementIDs())
	{
		const int meshSlot = polygonGroupToMaterialIndex[polyGroupID];
		staticMesh->GetSectionInfoMap().Set(0, meshSlot, FMeshSectionInfo(meshSlot));
	}
	staticMesh->CommitMeshDescription(0);
	staticMesh->SetLightMapCoordinateIndex(1);
	staticMesh->Build();
	staticMesh->CreateBodySetup();
	staticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;

	staticMesh->PostEditChange();
	FAssetRegistryModule::AssetCreated(staticMesh);
	staticMesh->MarkPackageDirty();

	return staticMesh;
}

AStaticMeshActor* FBSPImporter::RenderMeshToActor(const FMeshDescription& meshDesc, const FString& assetName, int lightmapResolution)
{
	UStaticMesh* staticMesh = RenderMeshToStaticMesh(meshDesc, assetName, lightmapResolution);

	AStaticMeshActor* staticMeshActor = world->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform::Identity);
	UStaticMeshComponent* staticMeshComponent = staticMeshActor->GetStaticMeshComponent();
	staticMeshComponent->SetStaticMesh(staticMesh);
	FLightmassPrimitiveSettings& lightmassSettings = staticMeshComponent->LightmassSettings;
	lightmassSettings.bUseEmissiveForStaticLighting = true;
	staticMeshComponent->bCastShadowAsTwoSided = true;

	staticMeshActor->PostEditChange();
	return staticMeshActor;
}

void FBSPImporter::GatherBrushes(uint32 nodeIndex, TArray<uint16>& out)
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
		GatherBrushes((uint32)node.m_Children[0], out);
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
		GatherBrushes((uint32)node.m_Children[1], out);
	}
}

void FBSPImporter::GatherFaces(uint32 nodeIndex, TArray<uint16>& out)
{
	const Valve::BSP::snode_t& node = bspFile.m_Nodes[nodeIndex];
	/*for (uint32 i = 0; i < node.m_Numfaces; ++i)
	{
		out.AddUnique(node.m_Firstface + i);
	}*/
	if (node.m_Children[0] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[0]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
		}
	}
	else
	{
		GatherFaces((uint32)node.m_Children[0], out);
	}
	if (node.m_Children[1] < 0)
	{
		const Valve::BSP::dleaf_t& leaf = bspFile.m_Leaves[-1 - node.m_Children[1]];
		for (uint32 i = 0; i < leaf.m_Numleaffaces; ++i)
		{
			out.AddUnique(bspFile.m_Leaffaces[leaf.m_Firstleafface + i]);
		}
	}
	else
	{
		GatherFaces((uint32)node.m_Children[1], out);
	}
}

void FBSPImporter::GatherDisplacements(const TArray<uint16>& faceIndices, TArray<uint16>& out)
{
	for (uint16 dispIndex = 0; dispIndex < (uint16)bspFile.m_Dispinfos.size(); ++dispIndex)
	{
		//const Valve::BSP::ddispinfo_t& bspDispInfo = bspFile.m_Dispinfos[dispIndex];
		//if (faceIndices.Contains(bspDispInfo.m_MapFace))
		{
			out.Add(dispIndex);
		}
	}
}

FPlane4f FBSPImporter::ValveToUnrealPlane(const Valve::BSP::cplane_t& plane)
{
	return SourceToUnreal.Plane(FPlane4f(plane.m_Normal(0, 0), plane.m_Normal(0, 1), plane.m_Normal(0, 2), plane.m_Distance));
}

void FBSPImporter::RenderFacesToMesh(const TArray<uint16>& faceIndices, FMeshDescription& meshDesc, bool skyboxFilter)
{
	TMap<uint32, FVertexID> valveToUnrealVertexMap;
	TMap<FName, FPolygonGroupID> materialToPolyGroupMap;
	TMap<FPolygonID, const Valve::BSP::dface_t*> polyToValveFaceMap;

	TAttributesSet<FVertexID>& vertexAttr = meshDesc.VertexAttributes();
	TMeshAttributesRef<FVertexID, FVector3f> vertexAttrPosition = vertexAttr.GetAttributesRef<FVector3f>(MeshAttribute::Vertex::Position);

	TAttributesSet<FVertexInstanceID>& vertexInstanceAttr = meshDesc.VertexInstanceAttributes();
	TMeshAttributesRef<FVertexInstanceID, FVector2f> vertexInstanceAttrUV = vertexInstanceAttr.GetAttributesRef<FVector2f>(MeshAttribute::VertexInstance::TextureCoordinate);
	TMeshAttributesRef<FVertexInstanceID, FVector4f> vertexInstanceAttrCol = vertexInstanceAttr.GetAttributesRef<FVector4f>(MeshAttribute::VertexInstance::Color);
	// vertexInstanceAttrUV.SetNumIndices(2);

	TAttributesSet<FEdgeID>& edgeAttr = meshDesc.EdgeAttributes();
	TMeshAttributesRef<FEdgeID, bool> edgeAttrIsHard = edgeAttr.GetAttributesRef<bool>(MeshAttribute::Edge::IsHard);

	TAttributesSet<FPolygonGroupID>& polyGroupAttr = meshDesc.PolygonGroupAttributes();
	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterial = polyGroupAttr.GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

	int cnt = 0;

	// Lookup faces
	TArray<const Valve::BSP::dface_t*> facesToRender;
	for (const uint16 faceIndex : faceIndices)
	{
		const Valve::BSP::dface_t& bspSplitFace = bspFile.m_Surfaces[faceIndex];
		facesToRender.Add(&bspSplitFace);
	}

	// Create geometry
	for (const Valve::BSP::dface_t* ptr : facesToRender)
	{
		const Valve::BSP::dface_t& bspFace = *ptr;
		if (bspFace.m_Dispinfo >= 0) { continue; }
		const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspFace.m_Texinfo];
		const uint16 texDataIndex = (uint16)bspTexInfo.m_Texdata;
		const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];
		const char* bspMaterialName = &bspFile.m_TexdataStringData[0] + bspFile.m_TexdataStringTable[bspTexData.m_NameStringTableID];
		FString parsedMaterialName = ParseMaterialName(bspMaterialName);

		const bool isSkybox = parsedMaterialName.Equals(TEXT("tools/toolsskybox"), ESearchCase::IgnoreCase) || parsedMaterialName.Equals(TEXT("tools/toolsskybox2d"), ESearchCase::IgnoreCase);
		if (isSkybox != skyboxFilter) { continue; }
		FName material(*parsedMaterialName);

		// Create polygroup if needed (we make one per material/texdata)
		FPolygonGroupID polyGroup;
		if (!materialToPolyGroupMap.Contains(material))
		{
			polyGroup = meshDesc.CreatePolygonGroup();
			materialToPolyGroupMap.Add(material, polyGroup);
			polyGroupMaterial[polyGroup] = material;
		}
		else
		{
			polyGroup = materialToPolyGroupMap[material];
		}

		TArray<FVertexInstanceID> polyVerts;
		polyVerts.Reserve(bspFace.m_Numedges);
		TSet<uint16> bspVerts;
		bspVerts.Reserve(bspFace.m_Numedges);

		// Iterate all edges
		for (uint16 i = 0; i < bspFace.m_Numedges; ++i)
		{
			const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
			const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);

			const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
			const uint16 vertIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];

			bool wasAlreadyInSet;
			bspVerts.Add(vertIndex, &wasAlreadyInSet);
			if (wasAlreadyInSet) { continue; }

			// Find or create vertex
			FVertexID vertID;
			{
				const FVertexID* vertPtr = valveToUnrealVertexMap.Find(vertIndex);
				if (vertPtr != nullptr)
				{
					vertID = *vertPtr;
				}
				else
				{
					vertID = meshDesc.CreateVertex();
					const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];
					vertexAttrPosition[vertID] = SourceToUnreal.Position(FVector3f(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2)));
					valveToUnrealVertexMap.Add(vertIndex, vertID);
				}
			}

			// Create vertex instance
			FVertexInstanceID vertInstID = meshDesc.CreateVertexInstance(vertID);

			// Calculate texture coords
			{
				const FVector3f pos = UnrealToSource.Position(vertexAttrPosition[vertID]);
				const FVector3f texU_XYZ = FVector3f(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2]);
				const float texU_W = bspTexInfo.m_TextureVecs[0][3];
				const FVector3f texV_XYZ = FVector3f(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2]);
				const float texV_W = bspTexInfo.m_TextureVecs[1][3];
				vertexInstanceAttrUV.Set(vertInstID, 0, FVector2f(
					(FVector3f::DotProduct(texU_XYZ, pos) + texU_W) / bspTexData.m_Width,
					(FVector3f::DotProduct(texV_XYZ, pos) + texV_W) / bspTexData.m_Height
				));
			}
			//{
			//	const FVector3f texU_XYZ = FVector3f(bspTexInfo.m_LightmapVecs[0][0], bspTexInfo.m_LightmapVecs[0][1], bspTexInfo.m_LightmapVecs[0][2]);
			//	const float texU_W = bspTexInfo.m_LightmapVecs[0][3];
			//	const FVector3f texV_XYZ = FVector3f(bspTexInfo.m_LightmapVecs[1][0], bspTexInfo.m_LightmapVecs[1][1], bspTexInfo.m_LightmapVecs[1][2]);
			//	const float texV_W = bspTexInfo.m_LightmapVecs[1][3];
			//	vertexInstanceAttrUV.Set(vertInst, 1, FVector2f(
			//		((FVector3f::DotProduct(texU_XYZ, pos) + texU_W)/* - bspFace.m_LightmapTextureMinsInLuxels[0]*/) / (bspFace.m_LightmapTextureSizeInLuxels[0] + 1),
			//		((FVector3f::DotProduct(texV_XYZ, pos) + texV_W)/* - bspFace.m_LightmapTextureMinsInLuxels[1]*/) / (bspFace.m_LightmapTextureSizeInLuxels[1] + 1)
			//	));
			//}

			// Default color
			vertexInstanceAttrCol[vertInstID] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);

			// Push
			polyVerts.Add(vertInstID);
		}

		// Create poly
		if (polyVerts.Num() > 2)
		{
			// Sort vertices to be convex
			const FPlane4f plane = ValveToUnrealPlane(bspFile.m_Planes[bspFace.m_Planenum]);
			const FQuat4f rot = FQuat4f::FindBetweenNormals(plane, FVector3f::UpVector);
			FVector3f centroid = FVector3f::ZeroVector;
			for (const FVertexInstanceID vertInstID : polyVerts)
			{
				centroid += vertexAttrPosition[meshDesc.GetVertexInstanceVertex(vertInstID)];
			}
			centroid /= polyVerts.Num();
			centroid = rot * centroid;
			polyVerts.Sort([&rot, &meshDesc, vertexAttrPosition, &centroid](FVertexInstanceID vertInstAID, FVertexInstanceID vertInstBID)
				{
					// a > b
					const FVector3f toA = (rot * vertexAttrPosition[meshDesc.GetVertexInstanceVertex(vertInstAID)]) - centroid;
					const FVector3f toB = (rot * vertexAttrPosition[meshDesc.GetVertexInstanceVertex(vertInstBID)]) - centroid;
					return FMath::Atan2(toA.Y, toA.X) > FMath::Atan2(toB.Y, toB.X);
				});

			// Create poly
			FPolygonID poly = meshDesc.CreatePolygon(polyGroup, polyVerts);
			polyToValveFaceMap.Add(poly, ptr);
		}
	}

	// Set smoothing groups
	TArray<FEdgeID> edges;
	for (const auto& pair : polyToValveFaceMap)
	{
		const Valve::BSP::dface_t& bspFace = *pair.Value;

		// Find all edges
		edges.Empty(3);
		meshDesc.GetPolygonPerimeterEdges(pair.Key, edges);
		for (const FEdgeID& edge : edges)
		{
			// Find polys connected to the edge
			const TArray<FPolygonID>& otherPolys = meshDesc.GetEdgeConnectedPolygons(edge);
			for (const FPolygonID& otherPoly : otherPolys)
			{
				if (otherPoly != pair.Key)
				{
					const Valve::BSP::dface_t& bspOtherFace = *polyToValveFaceMap[otherPoly];
					const bool isHard = !SharesSmoothingGroup(bspFace.m_SmoothingGroups, bspOtherFace.m_SmoothingGroups);
					edgeAttrIsHard[edge] = isHard;
				}
			}
		}
	}
}

void FBSPImporter::RenderBrushesToMesh(const TArray<uint16>& brushIndices, FMeshDescription& meshDesc)
{
	for (const uint16 brushIndex : brushIndices)
	{
		const Valve::BSP::dbrush_t& bspBrush = bspFile.m_Brushes[brushIndex];
		
		FBSPBrush brush;
		brush.Sides.Reserve(bspBrush.m_Numsides);
		for (int i = 0; i < bspBrush.m_Numsides; ++i)
		{
			const Valve::BSP::dbrushside_t& bspBrushSide = bspFile.m_Brushsides[bspBrush.m_Firstside + i];

			if (bspBrushSide.m_Bevel == 0)
			{
				const Valve::BSP::cplane_t& plane = bspFile.m_Planes[bspBrushSide.m_Planenum];
				FBSPBrushSide side;
				side.Plane = FPlane4f(plane.m_Normal(0, 0), plane.m_Normal(0, 1), plane.m_Normal(0, 2), plane.m_Distance);
				side.EmitGeometry = false;
				side.TextureU = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
				side.TextureV = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
				side.TextureW = 0;
				side.TextureH = 0;
				side.Material = NAME_None;
				if (bspBrushSide.m_Texinfo >= 0)
				{
					const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspBrushSide.m_Texinfo];
					constexpr int32 rejectedSurfFlags = Valve::BSP::SURF_NODRAW | Valve::BSP::SURF_SKY | Valve::BSP::SURF_SKY2D | Valve::BSP::SURF_SKIP | Valve::BSP::SURF_HINT;
					if (bspTexInfo.m_Texdata >= 0 && !(bspTexInfo.m_Flags & rejectedSurfFlags))
					{
						const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];
						const char* bspMaterialName = &bspFile.m_TexdataStringData[0] + bspFile.m_TexdataStringTable[bspTexData.m_NameStringTableID];
						FString parsedMaterialName = ParseMaterialName(bspMaterialName);
						side.TextureU = FVector4f(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2], bspTexInfo.m_TextureVecs[0][3]);
						side.TextureV = FVector4f(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2], bspTexInfo.m_TextureVecs[1][3]);
						side.TextureW = (uint16)bspTexData.m_Width;
						side.TextureH = (uint16)bspTexData.m_Height;
						side.Material = FName(*parsedMaterialName);
						side.EmitGeometry = true;
					}
				}
				if (bspBrushSide.m_Dispinfo > 0)
				{
					side.EmitGeometry = false;
				}
				side.SmoothingGroups = 0;
				brush.Sides.Add(side);
			}
		}

		FBSPBrushUtils::BuildBrushGeometry(brush, meshDesc);
	}
}

void FBSPImporter::RenderDisplacementsToMesh(const TArray<uint16>& displacements, FMeshDescription& meshDesc)
{
	FStaticMeshAttributes staticMeshAttr(meshDesc);

	TMeshAttributesRef<FVertexID, FVector3f> vertexAttrPosition = staticMeshAttr.GetVertexPositions();

	TMeshAttributesRef<FVertexInstanceID, FVector2f> vertexInstanceAttrUV = staticMeshAttr.GetVertexInstanceUVs();
	TMeshAttributesRef<FVertexInstanceID, FVector4f> vertexInstanceAttrCol = staticMeshAttr.GetVertexInstanceColors();

	TMeshAttributesRef<FEdgeID, bool> edgeAttrIsHard = staticMeshAttr.GetEdgeHardnesses();

	TMeshAttributesRef<FPolygonGroupID, FName> polyGroupMaterial = staticMeshAttr.GetPolygonGroupMaterialSlotNames();

	for (const uint16 dispIndex : displacements)
	{
		const Valve::BSP::ddispinfo_t& bspDispinfo = bspFile.m_Dispinfos[dispIndex];
		const Valve::BSP::dface_t& bspFace = bspFile.m_Surfaces[bspDispinfo.m_MapFace];
		const int dispRes = 1 << bspDispinfo.m_Power;
		const FVector3f dispStartPos(bspDispinfo.m_StartPosition(0, 0), bspDispinfo.m_StartPosition(0, 1), bspDispinfo.m_StartPosition(0, 2));

		const Valve::BSP::texinfo_t& bspTexInfo = bspFile.m_Texinfos[bspFace.m_Texinfo];
		const uint16 texDataIndex = (uint16)bspTexInfo.m_Texdata;
		const Valve::BSP::texdata_t& bspTexData = bspFile.m_Texdatas[bspTexInfo.m_Texdata];
		const char* bspMaterialName = &bspFile.m_TexdataStringData[0] + bspFile.m_TexdataStringTable[bspTexData.m_NameStringTableID];
		FString parsedMaterialName = ParseMaterialName(bspMaterialName);
		FName material(*parsedMaterialName);

		// Create a unique poly group for us
		const FPolygonGroupID polyGroupID = meshDesc.CreatePolygonGroup();
		polyGroupMaterial[polyGroupID] = material;

		// Gather face verts
		TArray<FVector3f> faceVerts;
		for (uint16 i = 0; i < bspFace.m_Numedges; ++i)
		{
			const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
			const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);

			const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
			const uint16 vertIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];

			const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];

			faceVerts.Add(FVector3f(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2)));
		}

		// Check that it has 4 verts (we can't map a displacement to anything other than a quad)
		if (faceVerts.Num() != 4)
		{
			UE_LOG(LogHL2BSPImporter, Warning, TEXT("Displacement by index $d is mapped to a non-quad face $d (with $d verts)"), dispIndex, bspDispinfo.m_MapFace, faceVerts.Num());
			continue;
		}

		// Find the face vert nearest the given starting position
		int startFaceVertIndex = 0;
		{
			FVector3f bestFaceVert = faceVerts[startFaceVertIndex];
			float bestFaceVertDist = FVector3f::DistSquared(bestFaceVert, dispStartPos);
			for (int i = startFaceVertIndex + 1; i < faceVerts.Num(); ++i)
			{
				const FVector3f faceVert = faceVerts[i];
				const float faceVertDist = FVector3f::DistSquared(faceVert, dispStartPos);
				if (faceVertDist < bestFaceVertDist)
				{
					bestFaceVert = faceVert;
					bestFaceVertDist = faceVertDist;
					startFaceVertIndex = i;
				}
			}
		}

		// Determine corners vectors
		const FVector3f p0 = faceVerts[startFaceVertIndex];
		const FVector3f p1 = faceVerts[(startFaceVertIndex + 1) % faceVerts.Num()];
		const FVector3f p2 = faceVerts[(startFaceVertIndex + 2) % faceVerts.Num()];
		const FVector3f p3 = faceVerts[(startFaceVertIndex + 3) % faceVerts.Num()];

		// Create all verts
		TArray<FVertexInstanceID> dispVertices;
		dispVertices.AddDefaulted((dispRes + 1) * (dispRes + 1));
		for (int x = 0; x <= dispRes; ++x)
		{
			const float dX = x / (float)dispRes;
			const FVector3f mp0 = FMath::Lerp(p0, p1, dX);
			const FVector3f mp1 = FMath::Lerp(p3, p2, dX);
			for (int y = 0; y <= dispRes; ++y)
			{
				const float dY = y / (float)dispRes;

				const int idx = x * (dispRes + 1) + y;
				const Valve::BSP::ddispvert_t& bspVert = bspFile.m_Dispverts[bspDispinfo.m_DispVertStart + idx];
				const FVector3f bspVertVec = FVector3f(bspVert.m_Vec(0, 0), bspVert.m_Vec(0, 1), bspVert.m_Vec(0, 2));

				const FVector3f basePos = FMath::Lerp(mp0, mp1, dY);
				const FVector3f dispPos = basePos + bspVertVec * bspVert.m_Dist;

				const FVertexID meshVertID = meshDesc.CreateVertex();
				vertexAttrPosition[meshVertID] = SourceToUnreal.Position(dispPos);

				const FVertexInstanceID meshVertInstID = meshDesc.CreateVertexInstance(meshVertID);

				const FVector3f texU_XYZ(bspTexInfo.m_TextureVecs[0][0], bspTexInfo.m_TextureVecs[0][1], bspTexInfo.m_TextureVecs[0][2]);
				const float texU_W = bspTexInfo.m_TextureVecs[0][3];
				const FVector3f texV_XYZ(bspTexInfo.m_TextureVecs[1][0], bspTexInfo.m_TextureVecs[1][1], bspTexInfo.m_TextureVecs[1][2]);
				const float texV_W = bspTexInfo.m_TextureVecs[1][3];
				vertexInstanceAttrUV.Set(meshVertInstID, 0, FVector2f(
					(FVector3f::DotProduct(texU_XYZ, basePos) + texU_W) / bspTexData.m_Width,
					(FVector3f::DotProduct(texV_XYZ, basePos) + texV_W) / bspTexData.m_Height
				));

				FVector4f col;
				col.X = FMath::Clamp(bspVert.m_Alpha / 255.0f, 0.0f, 1.0f);
				col.Y = 1.0f - col.X;
				col.Z = 0.0f;
				col.W = 1.0f;
				vertexInstanceAttrCol[meshVertInstID] = col;

				dispVertices[idx] = meshVertInstID;
			}
		}

		// Create all polys
		TArray<FVertexInstanceID> polyPoints;
		TArray<FEdgeID> polyEdgeIDs;
		for (int x = 0; x < dispRes; ++x)
		{
			for (int y = 0; y < dispRes; ++y)
			{
				const int i0 = x * (dispRes + 1) + y;
				const int i1 = (x + 1) * (dispRes + 1) + y;
				const int i2 = (x + 1) * (dispRes + 1) + (y + 1);
				const int i3 = x * (dispRes + 1) + (y + 1);

				polyPoints.Empty(4);

				if (SourceToUnreal.ShouldReverseWinding())
				{
					polyPoints.Add(dispVertices[i3]);
					polyPoints.Add(dispVertices[i2]);
					polyPoints.Add(dispVertices[i1]);
					polyPoints.Add(dispVertices[i0]);
				}
				else
				{
					polyPoints.Add(dispVertices[i0]);
					polyPoints.Add(dispVertices[i1]);
					polyPoints.Add(dispVertices[i2]);
					polyPoints.Add(dispVertices[i3]);
				}

				const FPolygonID polyID = meshDesc.CreatePolygon(polyGroupID, polyPoints);
				polyEdgeIDs.Empty(4);
				meshDesc.GetPolygonPerimeterEdges(polyID, polyEdgeIDs);
				for (const FEdgeID& edgeID : polyEdgeIDs)
				{
					edgeAttrIsHard[edgeID] = false;
				}
			}
		}
	}
}

void FBSPImporter::RenderTreeToVBSPInfo(uint32 nodeIndex)
{
	vbspInfo = world->SpawnActor<AVBSPInfo>();

	TMap<uint32, int> nodeMap;
	// TMap<uint32, int> leafMap;
	TMap<int16, int> clusterMap;

	struct ExploreState
	{
		uint32 NodeIndex;
		uint32 ParentNodeIndex;
		bool IsLeft;
		ExploreState(uint32 nodeIndex, uint32 parentNodeIndex, bool isLeft)
			: NodeIndex(nodeIndex), ParentNodeIndex(parentNodeIndex), IsLeft(isLeft)
		{ }
		ExploreState()
			: ExploreState(0, 0, false)
		{ }
	};

	// Explore the node hierarchy
	TArray<ExploreState> exploreStack;
	exploreStack.Push(ExploreState(nodeIndex, nodeIndex, false));
	while (exploreStack.Num() > 0)
	{
		const ExploreState state = exploreStack.Pop();
		const Valve::BSP::snode_t& bspNode = bspFile.m_Nodes[state.NodeIndex];
		FVBSPNode newNode;
		newNode.Plane = ValveToUnrealPlane(bspFile.m_Planes[bspNode.m_PlaneNum]);
		const int newNodeIndex = vbspInfo->Nodes.Add(newNode);
		nodeMap.Add(state.NodeIndex, newNodeIndex);
		if (state.ParentNodeIndex != state.NodeIndex)
		{
			FVBSPNode& parentNode = vbspInfo->Nodes[nodeMap[state.ParentNodeIndex]];
			if (state.IsLeft)
			{
				parentNode.Left = newNodeIndex;
			}
			else
			{
				parentNode.Right = newNodeIndex;
			}
		}

		// Check left
		if (bspNode.m_Children[0] < 0)
		{
			// Left is leaf
			const Valve::BSP::dleaf_t& bspLeaf = bspFile.m_Leaves[1 - bspNode.m_Children[0]];
			FVBSPLeaf newLeaf;
			newLeaf.Solid = (bspLeaf.m_Contents & Valve::BSP::CONTENTS_SOLID) != 0;
			int newLeafIndex = vbspInfo->Leaves.Num();
			if (bspLeaf.m_Cluster >= 0)
			{
				int* tmpCluster = clusterMap.Find(bspLeaf.m_Cluster);
				if (tmpCluster != nullptr)
				{
					newLeaf.Cluster = *tmpCluster;
				}
				else
				{
					FVBSPCluster newCluster;
					newCluster.Leaves.Add(newLeafIndex);
					newLeaf.Cluster = vbspInfo->Clusters.Add(newCluster);
					clusterMap.Add(bspLeaf.m_Cluster, newLeaf.Cluster);
				}
			}
			else
			{
				newLeaf.Cluster = -1;
			}
			check(vbspInfo->Leaves.Add(newLeaf) == newLeafIndex);
			vbspInfo->Nodes[newNodeIndex].Left = -(1 + newLeafIndex);
		}
		else
		{
			// Left is node
			exploreStack.Push(ExploreState((uint32)bspNode.m_Children[0], state.NodeIndex, true));
		}

		// Check right
		if (bspNode.m_Children[1] < 0)
		{
			// Right is leaf
			const Valve::BSP::dleaf_t& bspLeaf = bspFile.m_Leaves[1 - bspNode.m_Children[1]];
			FVBSPLeaf newLeaf;
			newLeaf.Solid = (bspLeaf.m_Contents & Valve::BSP::CONTENTS_SOLID) != 0;
			int newLeafIndex = vbspInfo->Leaves.Num();
			if (bspLeaf.m_Cluster >= 0)
			{
				int* tmpCluster = clusterMap.Find(bspLeaf.m_Cluster);
				if (tmpCluster != nullptr)
				{
					newLeaf.Cluster = *tmpCluster;
				}
				else
				{
					FVBSPCluster newCluster;
					newCluster.Leaves.Add(newLeafIndex);
					newLeaf.Cluster = vbspInfo->Clusters.Add(newCluster);
					clusterMap.Add(bspLeaf.m_Cluster, newLeaf.Cluster);
				}
			}
			else
			{
				newLeaf.Cluster = -1;
			}
			check(vbspInfo->Leaves.Add(newLeaf) == newLeafIndex);
			vbspInfo->Nodes[newNodeIndex].Right = -(1 + newLeafIndex);
		}
		else
		{
			// Right is node
			exploreStack.Push(ExploreState((uint32)bspNode.m_Children[1], state.NodeIndex, false));
		}
	}

	// Parse vis info
	for (const auto& pair : clusterMap)
	{
		const std::vector<int>& bspVisibleSet = bspFile.m_Visibility[pair.Key];
		FVBSPCluster& cluster = vbspInfo->Clusters[pair.Value];
		for (const int otherCluster : bspVisibleSet)
		{
			cluster.VisibleClusters.Add(otherCluster);
		}
	}

	vbspInfo->PostEditChange();
	vbspInfo->MarkPackageDirty();
}

float FBSPImporter::FindFaceArea(const Valve::BSP::dface_t& bspFace, bool unrealCoordSpace)
{
	TArray<FVector3f> vertices;
	for (int i = 0; i < bspFace.m_Numedges; ++i)
	{
		const int32 surfEdge = bspFile.m_Surfedges[bspFace.m_Firstedge + i];
		const uint32 edgeIndex = (uint32)(surfEdge < 0 ? -surfEdge : surfEdge);
		const Valve::BSP::dedge_t& bspEdge = bspFile.m_Edges[edgeIndex];
		const uint16 vertIndex = surfEdge < 0 ? bspEdge.m_V[1] : bspEdge.m_V[0];
		const Valve::BSP::mvertex_t& bspVertex = bspFile.m_Vertexes[vertIndex];
		const FVector3f pos(bspVertex.m_Position(0, 0), bspVertex.m_Position(0, 1), bspVertex.m_Position(0, 2));
		vertices.Add(unrealCoordSpace ? SourceToUnreal.Position(pos) : pos);
	}
	float area = 0.0f;
	for (int i = 2; i < vertices.Num(); ++i)
	{
		area += FMeshUtils::AreaOfTriangle(vertices[0], vertices[i - 1], vertices[i]);
	}
	return area;
}

FBox3f FBSPImporter::GetModelBounds(const Valve::BSP::dmodel_t& model, bool unrealCoordSpace)
{
	FVector3f p0(model.m_Mins(0, 0), model.m_Mins(0, 1), model.m_Mins(0, 2));
	FVector3f p1(model.m_Maxs(0, 0), model.m_Maxs(0, 1), model.m_Maxs(0, 2));
	if (unrealCoordSpace)
	{
		p0 = SourceToUnreal.Position(p0);
		p1 = SourceToUnreal.Position(p1);
	}
	const FVector3f min(FMath::Min(p0.X, p1.X), FMath::Min(p0.Y, p1.Y), FMath::Min(p0.Z, p1.Z));
	const FVector3f max(FMath::Max(p0.X, p1.X), FMath::Max(p0.Y, p1.Y), FMath::Max(p0.Z, p1.Z));
	return FBox3f(min, max);
}

FBox3f FBSPImporter::GetNodeBounds(const Valve::BSP::snode_t& node, bool unrealCoordSpace)
{
	FVector3f p0(node.m_Mins[0], node.m_Mins[1], node.m_Mins[2]);
	FVector3f p1(node.m_Maxs[0], node.m_Maxs[1], node.m_Maxs[2]);
	if (unrealCoordSpace)
	{
		p0 = SourceToUnreal.Position(p0);
		p1 = SourceToUnreal.Position(p1);
	}
	const FVector3f min(FMath::Min(p0.X, p1.X), FMath::Min(p0.Y, p1.Y), FMath::Min(p0.Z, p1.Z));
	const FVector3f max(FMath::Max(p0.X, p1.X), FMath::Max(p0.Y, p1.Y), FMath::Max(p0.Z, p1.Z));
	return FBox3f(min, max);
}

FString FBSPImporter::ParseMaterialName(const char* bspMaterialName)
{
	// It might be something like "brick/brick06c" which is fine
	// But it might also be like "maps/<mapname>/brick/brick06c_x_y_z" which is not fine
	// Also "nature/red_grass_wvt_patch" -> "nature/red_grass"
	// We need to identify the latter and convert it to the former
	FString bspMaterialNameAsStr(bspMaterialName);
	bspMaterialNameAsStr.ReplaceCharInline('\\', '/');
	{
		const static FRegexPattern patternCubemappedMaterial(TEXT("^maps[\\\\\\/]\\w+[\\\\\\/](.+)(?:_-?(?:\\d*\\.)?\\d+){3}$"));
		FRegexMatcher matchCubemappedMaterial(patternCubemappedMaterial, bspMaterialNameAsStr);
		if (matchCubemappedMaterial.FindNext())
		{
			bspMaterialNameAsStr = matchCubemappedMaterial.GetCaptureGroup(1);
		}
	}
	{
		const static FRegexPattern patternWVTMaterial(TEXT("^maps[\\\\\\/]\\w+[\\\\\\/](.+)_wvt_patch$"));
		FRegexMatcher matchWVTMaterial(patternWVTMaterial, bspMaterialNameAsStr);
		if (matchWVTMaterial.FindNext())
		{
			bspMaterialNameAsStr = matchWVTMaterial.GetCaptureGroup(1);
		}
	}
	return bspMaterialNameAsStr;
}

bool FBSPImporter::SharesSmoothingGroup(uint16 groupA, uint16 groupB)
{
	for (uint16 i = 0; i < 16; ++i)
	{
		const uint16 mask = 1 << i;
		if ((groupA & mask) && (groupB & mask)) { return true; }
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
