set(LIB voxelgenerator)
set(SRCS
	Spiral.h
	ShapeGenerator.h
	SpaceColonization.h SpaceColonization.cpp
	TreeType.h
	TreeGenerator.h TreeGenerator.cpp
	TreeContext.h
	NoiseGenerator.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelworld)
