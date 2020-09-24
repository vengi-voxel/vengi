set(SRCS
	common/CharacterMetaAttributes.h
	common/CharacterId.h
	common/Dummy.cpp
)
set(LIB ai-shared)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)
generate_protocol(${LIB} AIMessages.fbs)
