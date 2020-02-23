set(SRCS
	DepthBufferRenderer.cpp DepthBufferRenderer.h
	TestApp.cpp TestApp.h
	Movement.cpp Movement.h
	TestAppMain.h
)
set(FILES
	testcore/keybindings.cfg
)
engine_add_module(TARGET testcore FILES ${FILES} SRCS ${SRCS} DEPENDENCIES imgui render)
