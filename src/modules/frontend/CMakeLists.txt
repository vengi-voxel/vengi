fips_begin_module(frontend)
	fips_files(
		ClientEntity.h ClientEntity.cpp
		ClientEntityId.h
		MeshShader.h MeshShader.cpp
		WorldShader.h WorldShader.cpp
		WaterShader.h WaterShader.cpp
	)
	fips_deps(ui network voxel cooldown)
fips_end_module()
