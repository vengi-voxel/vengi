set(SRCS
	PoiProvider.cpp PoiProvider.h
	Type.h
)
set(LIB poi)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelworld network)

gtest_suite_sources(tests
	tests/PoiProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
