set(SRCS
	SoundManager.h SoundManager.cpp
)
set(LIB audio)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES io sdl2_mixer)
