fips_begin_app(worldgenerator cmdline)
	fips_files(
		WorldGenerator.cpp WorldGenerator.h
	)

	fips_dir(sauce)
	fips_files(
		WorldGeneratorModule.h
		WorldGeneratorInjector.h
	)

	fips_deps(voxel core io)
	copy_data_files(worldgenerator)
fips_end_app()
