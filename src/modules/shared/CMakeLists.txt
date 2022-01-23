set(LIB shared)
set(SRCS
	SharedMovement.cpp SharedMovement.h
	ProtocolEnum.h
)
engine_add_module(TARGET ${LIB} FILES ${FILES} SRCS ${SRCS} DEPENDENCIES voxelutil flatbuffers)
generate_protocol(${LIB} Shared.fbs ClientMessages.fbs ServerMessages.fbs)
