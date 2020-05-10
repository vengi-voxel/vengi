set(SRCS
	SharedMovement.cpp SharedMovement.h
)
engine_add_module(TARGET shared FILES ${FILES} SRCS ${SRCS} DEPENDENCIES core voxelutil network)
