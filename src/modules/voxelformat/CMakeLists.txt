set(LIB voxelformat)
set(SRCS
	Format.h Format.cpp
	AoSVXLFormat.h AoSVXLFormat.cpp
	BinVoxFormat.h BinVoxFormat.cpp
	CSMFormat.h CSMFormat.cpp
	KVXFormat.h KVXFormat.cpp
	KV6Format.h KV6Format.cpp
	GoxFormat.h GoxFormat.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	QBCLFormat.h QBCLFormat.cpp
	QEFFormat.h QEFFormat.cpp
	MCRFormat.h MCRFormat.cpp
	VXMFormat.h VXMFormat.cpp
	VXRFormat.h VXRFormat.cpp
	VXLFormat.h VXLFormat.cpp
	MeshCache.h MeshCache.cpp
	CubFormat.h CubFormat.cpp
	OBJFormat.h OBJFormat.cpp
	PLYFormat.h PLYFormat.cpp
	SproxelFormat.h SproxelFormat.cpp
	VolumeCache.h VolumeCache.cpp
	VoxelVolumes.h VoxelVolumes.cpp
	VoxFormat.h VoxFormat.cpp
	VoxOldFormat.h VoxOldFormat.cpp
	VolumeFormat.h VolumeFormat.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel voxelutil)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/AoSVXLFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/ConvertTest.cpp
	tests/CSMFormatTest.cpp
	tests/CubFormatTest.cpp
	tests/GoxFormatTest.cpp
	tests/KVXFormatTest.cpp
	tests/KV6FormatTest.cpp
	tests/MCRFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/QBCLFormatTest.cpp
	tests/QEFFormatTest.cpp
	tests/SproxelFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/VXLFormatTest.cpp
	tests/VXRFormatTest.cpp
	tests/VXMFormatTest.cpp
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
	tests/test.gox
	tests/test.vxm
	tests/test2.vxm
	tests/cw.cub
	tests/r.0.-2.mca
	tests/rgb.vxl
	tests/rgb.csv
	tests/rgb.vox
	tests/rgb.gox
	tests/rgb.qb
	tests/rgb.qef
	tests/rgb.cub
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
