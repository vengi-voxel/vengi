fips_begin_module(attrib)
	fips_files(
		Attributes.h Attributes.cpp
		Types.h
		Container.h Container.cpp
		ContainerProvider.h ContainerProvider.cpp
	)
	fips_deps(core commonlua)
fips_end_module()

begin_googleunittest(attrib)
	fips_dir(tests)
	fips_files(
		AttributesTest.cpp
	)
	fips_deps(attrib)
end_googleunittest()
