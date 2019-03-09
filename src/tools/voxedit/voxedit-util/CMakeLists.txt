set(SRCS
	ImportHeightmap.h ImportHeightmap.cpp
	MementoHandler.h MementoHandler.cpp

	ModifierType.h
	Config.h
	voxelizer.h

	tool/Fill.h tool/Fill.cpp
	tool/Crop.h tool/Crop.cpp
	tool/Resize.h tool/Resize.cpp
)
set(LIB voxedit-util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelrender voxelformat)

gtest_suite_sources(tests
	tests/MementoHandlerTest.cpp
)
gtest_suite_deps(tests ${LIB})
