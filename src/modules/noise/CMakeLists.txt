fips_begin_module(noise)
	fips_files(
		PerlinNoise.h PerlinNoise.cpp stb_perlin.h
		AccidentalNoise.h AccidentalNoise.cpp
		NoisePPNoise.h NoisePPNoise.cpp
		WorldShapeNoise.h WorldShapeNoise.cpp
	)
	fips_deps(core noisepp accidentalnoise)
fips_end_module()

gtest_begin(noise)
	fips_dir(tests)
	fips_files(
		AccidentalNoiseTest.cpp
		PerlinNoiseTest.cpp
		NoisePPNoiseTest.cpp
		NoiseTestHelper.h
	)
	fips_deps(core noise accidentalnoise)
gtest_end()
