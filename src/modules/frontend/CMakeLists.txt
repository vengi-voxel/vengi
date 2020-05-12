set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	ClientEntityRenderer.h ClientEntityRenderer.cpp
	Colors.h
	EntityMgr.cpp EntityMgr.h
	PlayerAction.h PlayerAction.cpp
	PlayerMovement.h PlayerMovement.cpp
)
set(FILES
	shared/sound/ambience_wind.wav

	shared/sound/footstep_generic.wav

	shared/sound/water_enter.wav
	shared/sound/water_leave.wav
)

set(LIB frontend)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES attrib stock animation shared audio)
