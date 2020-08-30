set(SRCS
	Attributes.h Attributes.cpp
	AttributeType.h
	Container.h Container.cpp
	ContainerProvider.h ContainerProvider.cpp
	ContainerValues.h
)
set(LIB attrib)
# the network module will assemble the header with the attribute types
# link it in because we need the directory where the c++ headers are
# generated to be also added to this lib.
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES commonlua network)

set(TEST_SRCS
	tests/AttributesTest.cpp
	tests/ContainerProviderTest.cpp
)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} image test-app)
gtest_suite_end(tests-${LIB})
