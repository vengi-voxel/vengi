set(LIB voxelgenerator)
set(SRCS
	Spiral.h
	ShapeGenerator.h
	SpaceColonization.h SpaceColonization.cpp
	TreeGenerator.h TreeGenerator.cpp
	NoiseGenerator.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES voxelworld)
