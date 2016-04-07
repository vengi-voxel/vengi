fips_begin_app(shapetool cmdline)
	fips_files(
		ShapeTool.h ShapeTool.cpp
	)

	fips_dir(sauce)
	fips_files(
		ShapeToolModule.h
		ShapeToolInjector.h
	)

	fips_dir(ui)
	fips_files(
		WorldParametersWindow.h
	)
	fips_deps(util frontend commonlua)
	copy_data_files(shapetool)
fips_end_app()
