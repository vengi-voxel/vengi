set(SRCS
	EventHandler.cpp EventHandler.h IEventObserver.h
	Filesystem.cpp Filesystem.h
	File.cpp File.h
	IOResource.h
)
set(LIB io)
add_library(${LIB} ${SRCS})
# TODO: pick one di framework
target_link_libraries(${LIB} sdl2 sauce fruit boostdi glm backward)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
