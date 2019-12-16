set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	Movement.h Movement.cpp
	PlayerMovement.h PlayerMovement.cpp
)

set(LIB frontend)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES attrib stock animation)
