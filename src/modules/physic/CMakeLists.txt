set(LIB physic)
set(SRCS
	Physic.cpp Physic.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core bullet)
