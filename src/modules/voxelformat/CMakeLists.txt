set(LIB voxelformat)
set(SRCS
	AoSVXLFormat.h AoSVXLFormat.cpp
	BinVoxFormat.h BinVoxFormat.cpp
	CSMFormat.h CSMFormat.cpp
	KVXFormat.h KVXFormat.cpp
	KV6Format.h KV6Format.cpp
	VoxFileFormat.h VoxFileFormat.cpp
	VoxFormat.h VoxFormat.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	QEFFormat.h QEFFormat.cpp
	VXMFormat.h VXMFormat.cpp
	VXLFormat.h VXLFormat.cpp
	MeshCache.h MeshCache.cpp
	CubFormat.h CubFormat.cpp
	OBJFormat.h OBJFormat.cpp
	PLYFormat.h PLYFormat.cpp
	VolumeCache.h VolumeCache.cpp
	VoxelVolumes.h VoxelVolumes.cpp
	VolumeFormat.h VolumeFormat.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel voxelutil)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/AoSVXLFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/QEFFormatTest.cpp
	tests/CubFormatTest.cpp
	tests/CSMFormatTest.cpp
	tests/KVXFormatTest.cpp
	tests/KV6FormatTest.cpp
	tests/VXLFormatTest.cpp
	tests/VXMFormatTest.cpp
)
set(TEST_FILES
	tests/qubicle.qb
	tests/qubicle.qbt
	tests/qubicle.qef
	tests/aceofspades.vxl
	tests/chronovox-studio.csm
	tests/cc.vxl
	tests/test.binvox
	tests/test.kvx
	tests/test.kv6
	tests/magicavoxel.vox
	tests/test.vxm
	tests/cw.cub
	tests/rgb.vxl
	tests/rgb.vox
	tests/rgb.qb
	tests/rgb.cub
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
