fips_begin_app(shapetool cmdline)
	fips_files(
		ShapeTool.h ShapeTool.cpp
	)

	fips_dir(sauce)
	fips_files(
		ShapeToolModule.h
		ShapeToolInjector.h
	)
	fips_deps(util frontend commonlua)
fips_end_app()
