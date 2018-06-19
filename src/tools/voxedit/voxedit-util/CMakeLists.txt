set(SRCS
	ImportHeightmap.h ImportHeightmap.cpp
	UndoHandler.h UndoHandler.cpp

	select/Edge.h select/Edge.cpp
	select/LineHorizontal.h select/LineHorizontal.cpp
	select/LineVertical.h select/LineVertical.cpp
	select/Same.h select/Same.cpp
	select/Single.h
	select/AABB.h select/AABB.cpp
	select/Select.h select/Select.cpp

	tool/Fill.h tool/Fill.cpp
	tool/Crop.h tool/Crop.cpp
	tool/Expand.h tool/Expand.cpp

	SelectionHandler.h SelectionHandler.cpp
	SelectType.h

	Shape.h
	ShapeHandler.h ShapeHandler.cpp
)
set(LIB voxedit-util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelrender voxelformat)

gtest_suite_sources(tests
	tests/UndoHandlerTest.cpp
)
gtest_suite_deps(tests ${LIB})
