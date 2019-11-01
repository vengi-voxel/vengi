set(SRCS
	TestApp.cpp TestApp.h
	TestAppMain.h
)
set(FILES
	testcore/keybindings.cfg
)
engine_add_module(TARGET testcore FILES ${FILES} SRCS ${SRCS} DEPENDENCIES frontend imgui)
