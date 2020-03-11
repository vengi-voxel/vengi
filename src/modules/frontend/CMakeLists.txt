set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	ClientEntityRenderer.h ClientEntityRenderer.cpp
	Colors.h
	PlayerAction.h PlayerAction.cpp
	PlayerMovement.h PlayerMovement.cpp
)

set(LIB frontend)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES attrib stock animation shared)
