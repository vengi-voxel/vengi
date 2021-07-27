set(SRCS
	TTY.cpp TTY.h
	TextConsole.cpp TextConsole.h
	TextConsoleApp.cpp TextConsoleApp.h
)
set(LIB console)

set(LIBS util app)

if (CURSES_FOUND)
	list(APPEND LIBS ${CURSES_LIBRARIES})
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

if (CURSES_FOUND)
	target_include_directories(${LIB} PRIVATE ${CURSES_INCLUDE_DIRS})
endif()
