set(LIB voxelformat)
set(SRCS
	external/ogt_vox.h
	external/tiny_obj_loader.h
	external/tiny_gltf.h
	external/json.hpp

	private/MinecraftPaletteMap.h private/MinecraftPaletteMap.cpp
	private/NamedBinaryTag.h private/NamedBinaryTag.cpp
	private/SchematicIntReader.h
	private/Tri.h private/Tri.cpp

	Format.h Format.cpp
	AoSVXLFormat.h AoSVXLFormat.cpp
	BinVoxFormat.h BinVoxFormat.cpp
	CSMFormat.h CSMFormat.cpp
	DatFormat.h DatFormat.cpp
	FBXFormat.h FBXFormat.cpp
	GoxFormat.h GoxFormat.cpp
	KVXFormat.h KVXFormat.cpp
	KV6Format.h KV6Format.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	QBCLFormat.h QBCLFormat.cpp
	QEFFormat.h QEFFormat.cpp
	MCRFormat.h MCRFormat.cpp
	VXAFormat.h VXAFormat.cpp
	VXCFormat.h VXCFormat.cpp
	VXMFormat.h VXMFormat.cpp
	VXRFormat.h VXRFormat.cpp
	VXTFormat.h VXTFormat.cpp
	VXLFormat.h VXLFormat.cpp
	MeshCache.h MeshCache.cpp
	MeshFormat.h MeshFormat.cpp
	CubFormat.h CubFormat.cpp
	OBJFormat.h OBJFormat.cpp
	PLYFormat.h PLYFormat.cpp
	SceneGraph.h SceneGraph.cpp
	SceneGraphNode.h SceneGraphNode.cpp
	SceneGraphUtil.h SceneGraphUtil.cpp
	SproxelFormat.h SproxelFormat.cpp
	SchematicFormat.h SchematicFormat.cpp
	SMFormat.h SMFormat.cpp
	STLFormat.h STLFormat.cpp
	VolumeCache.h VolumeCache.cpp
	VoxFormat.h VoxFormat.cpp
	SLAB6VoxFormat.h SLAB6VoxFormat.cpp
	VolumeFormat.h VolumeFormat.cpp
	GLTFFormat.h GLTFFormat.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelutil)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/ConvertTest.cpp
	tests/CSMFormatTest.cpp
	tests/CubFormatTest.cpp
	tests/GLTFFormatTest.cpp
	tests/GoxFormatTest.cpp
	tests/KVXFormatTest.cpp
	tests/KV6FormatTest.cpp
	tests/MeshFormatTest.cpp
	tests/MCRFormatTest.cpp
	tests/OBJFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/QBCLFormatTest.cpp
	tests/QEFFormatTest.cpp
	tests/SceneGraphTest.cpp
	tests/SceneGraphUtilTest.cpp
	tests/SchematicFormatTest.cpp
	tests/SproxelFormatTest.cpp
	tests/SMFormatTest.cpp
	tests/STLFormatTest.cpp
	tests/VolumeFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/VXLFormatTest.cpp
	tests/VXRFormatTest.cpp
	tests/VXMFormatTest.cpp

	tests/MinecraftPaletteMapTest.cpp
	tests/NamedBinaryTagTest.cpp
	tests/TriTest.cpp

	tests/8ontop.h
	tests/vox_character.h
	tests/vox_glasses.h
)
set(TEST_FILES
	tests/qubicle.qb
	tests/qubicle.qbt
	tests/qubicle.qef
	tests/qubicle.qbcl
	tests/testload.qef
	tests/aceofspades.vxl
	tests/chronovox-studio.csm
	tests/cc.vxl
	tests/test.binvox
	tests/test.kvx
	tests/test.kv6
	tests/minecraft_110.mca
	tests/minecraft_113.mca
	tests/magicavoxel.vox
	tests/cube.obj
	tests/cube.mtl
	tests/cube.stl
	tests/ascii.stl
	tests/test.gox
	tests/test.vxm
	tests/test2.vxm
	tests/8ontop.vox
	tests/cw.cub
	tests/r.0.-2.mca
	tests/r.0.-2.qb
	tests/rgb.vxl
	tests/rgb.csv
	tests/rgb.vox
	tests/rgb.gox
	tests/rgb.qb
	tests/rgb.qef
	tests/rgb.vxm
	tests/rgb.cub
	tests/rgb.qbcl
	tests/vox_character.vox
	tests/vox_glasses.vox
	tests/glTF/cube/Cube_BaseColor.png
	tests/glTF/cube/Cube_MetallicRoughness.png
	tests/glTF/cube/Cube.bin
	tests/glTF/cube/Cube.gltf
	tests/glTF/lantern/Lantern_baseColor.png
	tests/glTF/lantern/Lantern_emissive.png
	tests/glTF/lantern/Lantern_normal.png
	tests/glTF/lantern/Lantern_roughnessMetallic.png
	tests/glTF/lantern/Lantern.bin
	tests/glTF/lantern/Lantern.gltf
	voxedit/chr_knight.qb
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
