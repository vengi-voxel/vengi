fips_begin_module(frontend)
	fips_files(
		ClientEntity.h ClientEntity.cpp
		ClientEntityId.h
		Movement.h
		MeshShader.h MeshShader.cpp
		WorldRenderer.h WorldRenderer.cpp
		WorldShader.h WorldShader.cpp
		TerrainShader.h TerrainShader.cpp
		ColoredCubesShader.h ColoredCubesShader.cpp
	)
	fips_deps(ui network voxel cubiquity cooldown)
fips_end_module()
