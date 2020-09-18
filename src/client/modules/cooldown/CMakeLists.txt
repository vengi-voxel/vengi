set(SRCS
	CooldownHandler.h CooldownHandler.cpp
)
set(LIB client-cooldown)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES shared)
target_include_directories(${LIB} PUBLIC ..)
