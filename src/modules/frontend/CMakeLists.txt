set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	PlayerAction.h PlayerAction.cpp
	PlayerMovement.h PlayerMovement.cpp
)

set(LIB frontend)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES attrib stock animation shared)
