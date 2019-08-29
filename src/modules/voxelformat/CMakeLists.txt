set(LIB voxelformat)
set(SRCS
	BinVoxFormat.h BinVoxFormat.cpp
	VoxFileFormat.h VoxFileFormat.cpp
	VoxFormat.h VoxFormat.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	VXMFormat.h VXMFormat.cpp
	MeshExporter.h MeshExporter.cpp
	Loader.h Loader.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel assimp)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/VXMFormatTest.cpp
	tests/MeshExporterTest.cpp
)
set(TEST_FILES
	tests/qubicle.qb
	tests/qubicle.qbt
	tests/test.binvox
	tests/magicavoxel.vox
	tests/test.vxm
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
