fips_begin_module(frontend)
	fips_files(
		ClientEntity.h ClientEntity.cpp
		ClientEntityId.h
		MeshShader.h MeshShader.cpp
		WorldShader.h WorldShader.cpp
		WaterShader.h WaterShader.cpp
	)
	fips_dir(ui)
	fips_files(
		LoginWindow.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(ui network voxel cooldown ${SDL2_LIBRARIES})
fips_end_module()
