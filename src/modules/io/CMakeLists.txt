set(SRCS
	EventHandler.cpp EventHandler.h IEventObserver.h
	Filesystem.cpp Filesystem.h
	File.cpp File.h
	IOResource.h
)
set(LIB io)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} sdl2 sauce)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
