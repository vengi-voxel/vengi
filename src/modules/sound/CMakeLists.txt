set(SRCS
	SoundConfig.h
	SoundManager.h SoundManager.cpp
)

set(LIB sound)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core app)
