set(SRCS
	Attributes.h Attributes.cpp
	Types.h
	Container.h Container.cpp
	ContainerProvider.h ContainerProvider.cpp
)
set(LIB attrib)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core commonlua)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/AttributesTest.cpp
)
gtest_suite_deps(tests ${LIB})
