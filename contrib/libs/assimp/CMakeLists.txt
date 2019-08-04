set(SRCS
	code/3DS/3DSConverter.cpp
	code/3DS/3DSExporter.cpp
	code/3DS/3DSLoader.cpp
	code/3MF/D3MFExporter.cpp
	code/3MF/D3MFImporter.cpp
	code/3MF/D3MFOpcPackage.cpp
	code/AC/ACLoader.cpp
	code/ASE/ASELoader.cpp
	code/ASE/ASEParser.cpp
	code/Assbin/AssbinLoader.cpp
	code/Assbin/AssbinExporter.cpp
	code/Assjson/cencode.c
	code/Assjson/json_exporter.cpp
	code/Assjson/mesh_splitter.cpp
	code/Assxml/AssxmlExporter.cpp
	code/B3D/B3DImporter.cpp
	code/Blender/BlenderBMesh.cpp
	code/Blender/BlenderDNA.cpp
	code/Blender/BlenderLoader.cpp
	code/Blender/BlenderModifier.cpp
	code/Blender/BlenderScene.cpp
	code/Blender/BlenderTessellator.cpp
	code/BVH/BVHLoader.cpp
	code/CApi/AssimpCExport.cpp
	code/CApi/CInterfaceIOWrapper.cpp
	code/COB/COBLoader.cpp
	code/Collada/ColladaExporter.cpp
	code/Collada/ColladaLoader.cpp
	code/Collada/ColladaParser.cpp
	code/Common/Assimp.cpp
	code/Common/BaseImporter.cpp
	code/Common/BaseProcess.cpp
	code/Common/Bitmap.cpp
	code/Common/CreateAnimMesh.cpp
	code/Common/DefaultIOStream.cpp
	code/Common/DefaultIOSystem.cpp
	code/Common/DefaultLogger.cpp
	code/Common/Exporter.cpp
	code/Common/Importer.cpp
	code/Common/ImporterRegistry.cpp
	code/Common/PostStepRegistry.cpp
	code/Common/RemoveComments.cpp
	code/Common/SceneCombiner.cpp
	code/Common/scene.cpp
	code/Common/ScenePreprocessor.cpp
	code/Common/SGSpatialSort.cpp
	code/Common/SkeletonMeshBuilder.cpp
	code/Common/SpatialSort.cpp
	code/Common/SplitByBoneCountProcess.cpp
	code/Common/StandardShapes.cpp
	code/Common/Subdivision.cpp
	code/Common/TargetAnimation.cpp
	code/Common/Version.cpp
	code/Common/VertexTriangleAdjacency.cpp
	code/CSM/CSMLoader.cpp
	code/DXF/DXFLoader.cpp
	code/FBX/FBXConverter.cpp
	code/FBX/FBXAnimation.cpp
	code/FBX/FBXBinaryTokenizer.cpp
	code/FBX/FBXDeformer.cpp
	code/FBX/FBXDocument.cpp
	code/FBX/FBXDocumentUtil.cpp
	code/FBX/FBXExporter.cpp
	code/FBX/FBXExportNode.cpp
	code/FBX/FBXExportProperty.cpp
	code/FBX/FBXImporter.cpp
	code/FBX/FBXMaterial.cpp
	code/FBX/FBXMeshGeometry.cpp
	code/FBX/FBXModel.cpp
	code/FBX/FBXNodeAttribute.cpp
	code/FBX/FBXParser.cpp
	code/FBX/FBXProperties.cpp
	code/FBX/FBXTokenizer.cpp
	code/FBX/FBXUtil.cpp
	code/glTF2/glTF2Importer.cpp
	code/glTF2/glTF2Exporter.cpp
	code/glTF/glTFExporter.cpp
	code/glTF/glTFImporter.cpp
	code/HMP/HMPLoader.cpp
	code/Importer/IFC/IFCBoolean.cpp
	code/Importer/IFC/IFCCurve.cpp
	code/Importer/IFC/IFCGeometry.cpp
	code/Importer/IFC/IFCLoader.cpp
	code/Importer/IFC/IFCMaterial.cpp
	code/Importer/IFC/IFCOpenings.cpp
	code/Importer/IFC/IFCProfile.cpp
	code/Importer/IFC/IFCReaderGen1_2x3.cpp
	code/Importer/IFC/IFCReaderGen2_2x3.cpp
	code/Importer/IFC/IFCReaderGen_4.cpp
	code/Importer/IFC/IFCUtil.cpp
	code/Importer/STEPParser/STEPFileEncoding.cpp
	code/Importer/STEPParser/STEPFileReader.cpp
	code/Importer/StepFile/StepFileImporter.cpp
	code/Importer/StepFile/StepFileGen1.cpp
	code/Importer/StepFile/StepFileGen2.cpp
	code/Importer/StepFile/StepFileGen3.cpp
	code/Irr/IRRLoader.cpp
	code/Irr/IRRMeshLoader.cpp
	code/Irr/IRRShared.cpp
	code/LWO/LWOAnimation.cpp
	code/LWO/LWOBLoader.cpp
	code/LWO/LWOLoader.cpp
	code/LWO/LWOMaterial.cpp
	code/LWS/LWSLoader.cpp
	code/Material/MaterialSystem.cpp
	code/MD2/MD2Loader.cpp
	code/MD3/MD3Loader.cpp
	code/MD5/MD5Loader.cpp
	code/MD5/MD5Parser.cpp
	code/MDC/MDCLoader.cpp
	code/MDL/MDLLoader.cpp
	code/MDL/MDLMaterialLoader.cpp
	code/MS3D/MS3DLoader.cpp
	code/NDO/NDOLoader.cpp
	code/NFF/NFFLoader.cpp
	code/Obj/ObjExporter.cpp
	code/Obj/ObjFileImporter.cpp
	code/Obj/ObjFileMtlImporter.cpp
	code/Obj/ObjFileParser.cpp
	code/OFF/OFFLoader.cpp
	code/Ogre/OgreBinarySerializer.cpp
	code/Ogre/OgreImporter.cpp
	code/Ogre/OgreMaterial.cpp
	code/Ogre/OgreStructs.cpp
	code/Ogre/OgreXmlSerializer.cpp
	code/OpenGEX/OpenGEXExporter.cpp
	code/OpenGEX/OpenGEXImporter.cpp
	code/Ply/PlyExporter.cpp
	code/Ply/PlyLoader.cpp
	code/Ply/PlyParser.cpp
	code/PostProcessing/CalcTangentsProcess.cpp
	code/PostProcessing/ComputeUVMappingProcess.cpp
	code/PostProcessing/ConvertToLHProcess.cpp
	code/PostProcessing/DeboneProcess.cpp
	code/PostProcessing/DropFaceNormalsProcess.cpp
	code/PostProcessing/EmbedTexturesProcess.cpp
	code/PostProcessing/FindDegenerates.cpp
	code/PostProcessing/FindInstancesProcess.cpp
	code/PostProcessing/FindInvalidDataProcess.cpp
	code/PostProcessing/FixNormalsStep.cpp
	code/PostProcessing/GenBoundingBoxesProcess.cpp
	code/PostProcessing/GenFaceNormalsProcess.cpp
	code/PostProcessing/GenVertexNormalsProcess.cpp
	code/PostProcessing/ImproveCacheLocality.cpp
	code/PostProcessing/JoinVerticesProcess.cpp
	code/PostProcessing/LimitBoneWeightsProcess.cpp
	code/PostProcessing/MakeVerboseFormat.cpp
	code/PostProcessing/OptimizeGraph.cpp
	code/PostProcessing/OptimizeMeshes.cpp
	code/PostProcessing/PretransformVertices.cpp
	code/PostProcessing/ProcessHelper.cpp
	code/PostProcessing/RemoveRedundantMaterials.cpp
	code/PostProcessing/RemoveVCProcess.cpp
	code/PostProcessing/ScaleProcess.cpp
	code/PostProcessing/SortByPTypeProcess.cpp
	code/PostProcessing/SplitLargeMeshes.cpp
	code/PostProcessing/TextureTransform.cpp
	code/PostProcessing/TriangulateProcess.cpp
	code/PostProcessing/ValidateDataStructure.cpp
	code/Q3BSP/Q3BSPFileImporter.cpp
	code/Q3BSP/Q3BSPFileParser.cpp
	code/Q3BSP/Q3BSPZipArchive.cpp
	code/Q3D/Q3DLoader.cpp
	code/Raw/RawLoader.cpp
	code/SIB/SIBImporter.cpp
	code/SMD/SMDLoader.cpp
	code/Step/StepExporter.cpp
	code/STL/STLExporter.cpp
	code/STL/STLLoader.cpp
	code/Terragen/TerragenLoader.cpp
	code/Unreal/UnrealLoader.cpp
	code/X3D/X3DExporter.cpp
	code/X3D/X3DImporter.cpp
	code/X3D/X3DImporter_Geometry2D.cpp
	code/X3D/X3DImporter_Geometry3D.cpp
	code/X3D/X3DImporter_Group.cpp
	code/X3D/X3DImporter_Light.cpp
	code/X3D/X3DImporter_Metadata.cpp
	code/X3D/X3DImporter_Networking.cpp
	code/X3D/X3DImporter_Postprocess.cpp
	code/X3D/X3DImporter_Rendering.cpp
	code/X3D/X3DImporter_Shape.cpp
	code/X3D/X3DImporter_Texturing.cpp
	code/XGL/XGLLoader.cpp
	code/X/XFileExporter.cpp
	code/X/XFileImporter.cpp
	code/X/XFileParser.cpp
	contrib/clipper/clipper.cpp
	contrib/irrXML/irrXML.cpp
	contrib/openddlparser/code/DDLNode.cpp
	contrib/openddlparser/code/OpenDDLCommon.cpp
	contrib/openddlparser/code/OpenDDLExport.cpp
	contrib/openddlparser/code/OpenDDLParser.cpp
	contrib/openddlparser/code/Value.cpp
	contrib/unzip/ioapi.c
	contrib/unzip/unzip.c
	contrib/zip/src/zip.c)
#	code/C4D/C4DImporter.cpp

engine_add_library(
	LIB assimp
	SRCS ${SRCS}
)

if (NOT ASSIMP_FOUND)
	target_link_libraries(assimp zlib)
	target_include_directories(assimp PUBLIC include/)
	target_include_directories(assimp PRIVATE include/assimp/)
	target_include_directories(assimp PRIVATE ./../)
	target_include_directories(assimp PRIVATE code/)
	target_include_directories(assimp PRIVATE contrib/openddlparser/include/)
	target_include_directories(assimp PRIVATE contrib/rapidjson/include/)
	target_include_directories(assimp PRIVATE contrib/irrXML/)
	target_include_directories(assimp PRIVATE ${CMAKE_BINARY_DIR}/assimp/)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_X_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3DS_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MD3_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MMD_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MDL_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MD2_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_PLY_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_ASE_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OBJ_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_HMP_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_SMD_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MDC_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MD5_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_STL_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_LWO_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_DXF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_NFF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_RAW_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_SIB_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OFF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_AC_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_BVH_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_IRRMESH_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_IRR_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_Q3D_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_B3D_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_COLLADA_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_TERRAGEN_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_CSM_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3D_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_LWS_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OGRE_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OPENGEX_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MS3D_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_COB_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_BLEND_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_Q3BSP_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_NDO_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_IFC_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_XGL_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_FBX_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_ASSBIN_IMPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_GLTF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_C4D_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3MF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_AMF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_X3D_IMPORTER)

	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_ASSJSON_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_FBX_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_COLLADA_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_XFILE_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_STEP_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OBJ_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_STL_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_PLY_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3DS_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_GLTF_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_ASSBIN_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_ASSXML_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_X3D_EXPORTER)

	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OWN_ZLIB)
	target_compile_definitions(assimp PRIVATE -DOPENDDLPARSER_BUILD=1)

	set(ASSIMP_BUILD_TEST NO)
	configure_file(include/assimp/config.h.in ${CMAKE_BINARY_DIR}/assimp/config.h)

	if(MSVC)
		set_target_properties(assimp PROPERTIES COMPILE_FLAGS "/wd4267 /wd4152 /wd4204 /wd4242 /wd4244 /wd4668 /wd4996")
	elseif (CMAKE_COMPILER_IS_GNUCC)
		set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-tautological-compare -Wno-char-subscripts -Wno-class-memaccess -Wno-misleading-indentation -Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-reorder -Wno-ignored-qualifiers -Wno-unused-but-set-variable -fexceptions -frtti")
	else()
		set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-tautological-compare -Wno-char-subscripts -Wno-class-memaccess -Wno-misleading-indentation -Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-reorder -Wno-ignored-qualifiers -Wno-address-of-packed-member -Wno-undefined-var-template -Wno-switch -Wno-char-subscripts -Wno-incompatible-pointer-types -fexceptions -frtti")
	endif()
endif()
