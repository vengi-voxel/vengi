fips_begin_module(attrib)
	fips_files(
		Attributes.h Attributes.cpp
		Types.h
		Container.h Container.cpp
		ContainerProvider.h ContainerProvider.cpp
	)
	fips_deps(core commonlua)
fips_end_module()

gtest_suite_files(tests tests/AttributesTest.cpp)
gtest_suite_deps(tests attrib)
