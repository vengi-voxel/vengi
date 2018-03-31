set(SRCS
	PoiProvider.cpp PoiProvider.h
)
set(LIB poi)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES voxel network)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/PoiProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
