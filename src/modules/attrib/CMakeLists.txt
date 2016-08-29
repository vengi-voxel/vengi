set(SRCS
	Attributes.h Attributes.cpp
	AttributeType.h
	Container.h Container.cpp
	ContainerProvider.h ContainerProvider.cpp
)
set(LIB attrib)
add_library(${LIB} ${SRCS})
# the network module will assemble the header with the attribute types
# link it in because we need the directory where the c++ headers are
# generated to be also added to this lib. TODO: find a way to just
# use add_dependencies
#add_dependencies(${LIB} network)
target_link_libraries(${LIB} core commonlua network)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/AttributesTest.cpp
)
gtest_suite_deps(tests ${LIB})
