set(SRCS
	TestApp.cpp TestApp.h
	Plane.h Plane.cpp
	TextureRenderer.cpp TextureRenderer.h
	TestAppMain.h
)
set(FILES
	testcore/keybindings.cfg
)
engine_add_module(TARGET testcore FILES ${FILES} SRCS ${SRCS} DEPENDENCIES ui)
