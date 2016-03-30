fips_begin_app(shadertool cmdline)
	fips_files(
		ShaderTool.h ShaderTool.cpp
	)

	fips_dir(sauce)
	fips_files(
		ShaderToolModule.h
		ShaderToolInjector.h
	)
	fips_deps(core video)
fips_end_app()
