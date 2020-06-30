set(LIB voxelformat)
set(SRCS
	AoSVXLFormat.h AoSVXLFormat.cpp
	BinVoxFormat.h BinVoxFormat.cpp
	KVXFormat.h KVXFormat.cpp
	KV6Format.h KV6Format.cpp
	VoxFileFormat.h VoxFileFormat.cpp
	VoxFormat.h VoxFormat.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	VXMFormat.h VXMFormat.cpp
	VXLFormat.h VXLFormat.cpp
	MeshCache.h MeshCache.cpp
	CubFormat.h CubFormat.cpp
	VolumeCache.h VolumeCache.cpp
	VoxelVolumes.h VoxelVolumes.cpp
	Loader.h Loader.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel voxelutil)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/AoSVXLFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/CubFormatTest.cpp
	tests/KVXFormatTest.cpp
	tests/KV6FormatTest.cpp
	tests/VXLFormatTest.cpp
	tests/VXMFormatTest.cpp
)
set(TEST_FILES
	tests/qubicle.qb
	tests/qubicle.qbt
	tests/aceofspades.vxl
	tests/cc.vxl
	tests/test.binvox
	tests/test.kvx
	tests/test.kv6
	tests/magicavoxel.vox
	tests/test.vxm
	tests/cw.cub
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
