set(SRCS
	Image.cpp Image.h
)
set(LIB image)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core io)
