set(SRCS
	Attributes.h Attributes.cpp
	AttributeType.h
	Container.h Container.cpp
	ContainerProvider.h ContainerProvider.cpp
	ContainerValues.h
	LUAContainer.h LUAContainer.cpp
)
set(LIB attrib)
# the network module will assemble the header with the attribute types
# link it in because we need the directory where the c++ headers are
# generated to be also added to this lib.
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES commonlua network)

gtest_suite_sources(tests
	tests/AttributesTest.cpp
	tests/ContainerProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
