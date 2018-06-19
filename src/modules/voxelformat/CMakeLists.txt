set(LIB voxelformat)
set(SRCS
	VoxFileFormat.h VoxFileFormat.cpp
	VoxFormat.h VoxFormat.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	MeshExporter.h MeshExporter.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel assimp)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/MeshExporterTest.cpp
)
gtest_suite_files(tests tests/qubicle.qb tests/magicavoxel.vox)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
