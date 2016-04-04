fips_begin_module(image)
	fips_files(
		Image.cpp Image.h
	)
	fips_deps(core io)
fips_end_module()
