set(SRCS
	code/FixNormalsStep.cpp
	code/IFCLoader.cpp
	code/Importer.cpp
	code/RemoveRedundantMaterials.cpp
	code/UnrealLoader.cpp
	code/IFCCurve.cpp
	code/FBXNodeAttribute.cpp
	code/Subdivision.cpp
	code/OptimizeMeshes.cpp
	code/FBXProperties.cpp
	code/BlenderScene.cpp
	code/CInterfaceIOWrapper.cpp
	code/StepExporter.cpp
	code/OgreStructs.cpp
	code/Version.cpp
	code/BaseImporter.cpp
	code/CSMLoader.cpp
	code/SMDLoader.cpp
	code/IFCReaderGen1.cpp
	code/IFCReaderGen2.cpp
	code/FindInvalidDataProcess.cpp
	code/ScenePreprocessor.cpp
	code/BVHLoader.cpp
	code/ColladaParser.cpp
	code/RemoveVCProcess.cpp
	code/LWOAnimation.cpp
	code/ColladaExporter.cpp
	code/LWOBLoader.cpp
	code/FBXDeformer.cpp
	code/SkeletonMeshBuilder.cpp
	code/RawLoader.cpp
	code/IFCBoolean.cpp
	code/FBXMeshGeometry.cpp
	code/ImporterRegistry.cpp
	code/ValidateDataStructure.cpp
	code/Q3BSPZipArchive.cpp
	code/ObjFileImporter.cpp
	code/RemoveComments.cpp
	code/ConvertToLHProcess.cpp
	code/MS3DLoader.cpp
	code/SplitByBoneCountProcess.cpp
	code/SGSpatialSort.cpp
	code/OpenGEXExporter.cpp
	code/MDLMaterialLoader.cpp
	code/IRRMeshLoader.cpp
	code/FBXDocumentUtil.cpp
	code/D3MFOpcPackage.cpp
	code/ObjFileParser.cpp
	code/OgreBinarySerializer.cpp
	code/IFCProfile.cpp
	code/DXFLoader.cpp
	code/PlyParser.cpp
	code/COBLoader.cpp
	code/LWSLoader.cpp
	code/PlyLoader.cpp
	code/XFileExporter.cpp
	code/FBXAnimation.cpp
	code/TriangulateProcess.cpp
	code/ObjExporter.cpp
	code/ColladaLoader.cpp
	code/TargetAnimation.cpp
	code/IFCUtil.cpp
	code/FindDegenerates.cpp
	code/PretransformVertices.cpp
	code/BlenderLoader.cpp
	code/IFCOpenings.cpp
	code/MaterialSystem.cpp
	code/glTFImporter.cpp
	code/AssbinLoader.cpp
	code/ComputeUVMappingProcess.cpp
	code/glTFExporter.cpp
	code/Q3BSPFileParser.cpp
	code/B3DImporter.cpp
	code/MD5Parser.cpp
	code/IFCMaterial.cpp
	code/MD5Loader.cpp
	code/GenFaceNormalsProcess.cpp
	code/AssxmlExporter.cpp
	code/MDCLoader.cpp
	code/FBXMaterial.cpp
	code/STEPFileReader.cpp
	code/ASELoader.cpp
	code/MDLLoader.cpp
	code/CalcTangentsProcess.cpp
	code/IFCGeometry.cpp
	code/PlyExporter.cpp
	code/IRRShared.cpp
	code/NDOLoader.cpp
	code/ACLoader.cpp
	code/LimitBoneWeightsProcess.cpp
	code/SIBImporter.cpp
	code/TextureTransform.cpp
	code/OFFLoader.cpp
	code/OgreXmlSerializer.cpp
	code/OgreImporter.cpp
	code/SceneCombiner.cpp
	code/FBXDocument.cpp
	code/FBXConverter.cpp
	code/DeboneProcess.cpp
	code/LWOLoader.cpp
	code/FBXTokenizer.cpp
	code/Q3BSPFileImporter.cpp
	code/HMPLoader.cpp
	code/BlenderDNA.cpp
	code/DefaultLogger.cpp
	code/DefaultIOStream.cpp
	code/IRRLoader.cpp
	code/TerragenLoader.cpp
	code/AssbinExporter.cpp
	code/XGLLoader.cpp
	code/OpenGEXImporter.cpp
	code/SpatialSort.cpp
	code/3DSConverter.cpp
	code/Exporter.cpp
	code/BaseProcess.cpp
	code/SortByPTypeProcess.cpp
	code/FBXImporter.cpp
	code/FindInstancesProcess.cpp
	code/ASEParser.cpp
	code/BlenderBMesh.cpp
	code/SplitLargeMeshes.cpp
	code/D3MFImporter.cpp
	code/XFileImporter.cpp
	code/3DSExporter.cpp
	code/BlenderModifier.cpp
	code/ProcessHelper.cpp
	code/BlenderTessellator.cpp
	code/STEPFileEncoding.cpp
	code/OptimizeGraph.cpp
	code/DefaultIOSystem.cpp
	code/GenVertexNormalsProcess.cpp
	code/3DSLoader.cpp
	code/AssimpCExport.cpp
	code/STLLoader.cpp
	code/VertexTriangleAdjacency.cpp
	code/MD2Loader.cpp
	code/FBXModel.cpp
	code/JoinVerticesProcess.cpp
	code/LWOMaterial.cpp
	code/FBXBinaryTokenizer.cpp
	code/FBXUtil.cpp
	code/PostStepRegistry.cpp
	code/ObjFileMtlImporter.cpp
	code/Bitmap.cpp
	code/XFileParser.cpp
	code/Assimp.cpp
	code/StandardShapes.cpp
	code/Q3DLoader.cpp
	code/FBXParser.cpp
	code/ImproveCacheLocality.cpp
	code/STLExporter.cpp
	code/MD3Loader.cpp
	code/OgreMaterial.cpp
	code/MakeVerboseFormat.cpp
	code/NFFLoader.cpp
	code/X3DExporter.cpp
	code/X3DExporter.hpp
	code/X3DImporter.cpp
	code/X3DImporter.hpp
	code/X3DImporter_Geometry2D.cpp
	code/X3DImporter_Geometry3D.cpp
	code/X3DImporter_Group.cpp
	code/X3DImporter_Light.cpp
	code/X3DImporter_Macro.hpp
	code/X3DImporter_Metadata.cpp
	code/X3DImporter_Networking.cpp
	code/X3DImporter_Node.hpp
	code/X3DImporter_Postprocess.cpp
	code/X3DImporter_Rendering.cpp
	code/X3DImporter_Shape.cpp
	code/X3DImporter_Texturing.cpp
	contrib/openddlparser/code/DDLNode.cpp
	contrib/openddlparser/code/Value.cpp
	contrib/openddlparser/code/OpenDDLExport.cpp
	contrib/openddlparser/code/OpenDDLParser.cpp
	contrib/openddlparser/code/OpenDDLCommon.cpp
	contrib/clipper/clipper.cpp
	contrib/unzip/unzip.c
	contrib/unzip/ioapi.c
	contrib/ConvertUTF/ConvertUTF.c
	contrib/irrXML/irrXML.cpp
)
#	code/C4DImporter.cpp

engine_add_library(
	LIB assimp
	SRCS ${SRCS}
)

if (NOT ASSIMP_FOUND)
	add_dependencies(assimp zlib)
	if (ZLIB_LIBRARIES)
		target_link_libraries(assimp ${ZLIB_LIBRARIES})
	endif()
	target_include_directories(assimp PUBLIC include/)
	if (ZLIB_INCLUDE_DIRS)
		target_include_directories(assimp PRIVATE ${ZLIB_INCLUDE_DIRS})
	endif()
	target_include_directories(assimp PRIVATE include/assimp/)
	target_include_directories(assimp PRIVATE ./../)
	target_include_directories(assimp PRIVATE code/)
	target_include_directories(assimp PRIVATE contrib/openddlparser/include/)
	target_include_directories(assimp PRIVATE contrib/rapidjson/include/)
	target_include_directories(assimp PRIVATE ${CMAKE_BINARY_DIR}/assimp/)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_X_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3DS_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_MD3_IMPORTER)
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
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_GLTF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_C4D_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3MF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_AMF_IMPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_X3D_IMPORTER)

	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_COLLADA_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_XFILE_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_STEP_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_OBJ_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_STL_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_PLY_EXPORTER)
	#target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_3DS_EXPORTER)
	target_compile_definitions(assimp PRIVATE -DASSIMP_BUILD_NO_GLTF_EXPORTER)
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
		set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-reorder -Wno-ignored-qualifiers -Wno-unused-but-set-variable")
	else()
		set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-reorder -Wno-ignored-qualifiers -Wno-address-of-packed-member -Wno-undefined-var-template -Wno-switch -Wno-char-subscripts -Wno-incompatible-pointer-types")
	endif()
endif()
