fips_begin_app(cubiquitytool windowed)
	fips_files(
		CubiquityTool.h CubiquityTool.cpp
	)

	fips_dir(sauce)
	fips_files(
		CubiquityToolModule.h
		CubiquityToolInjector.h
	)
	fips_deps(core video frontend)
	copy_data_files(cubiquitytool)
fips_end_app()
