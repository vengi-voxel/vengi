set(SRCS
	EventHandler.cpp EventHandler.h IEventObserver.h
	Filesystem.cpp Filesystem.h
	File.cpp File.h
	IOResource.h
)
set(LIB io)
add_library(${LIB} ${SRCS})
# TODO: pick one di framework
set(EXTRA_LIBS "")
if (NOT MSVC AND NOT DARWIN)
	# TODO: stdc++fs is only valid for gcc until c++17 shipped, then it is part of the standard lib
	list(APPEND EXTRA_LIBS stdc++fs)
endif()
target_link_libraries(${LIB} sdl2 glm backward ${EXTRA_LIBS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
