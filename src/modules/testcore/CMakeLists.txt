set(SRCS
	TestApp.cpp TestApp.h
	Movement.cpp Movement.h
	Plane.h Plane.cpp
	TextureRenderer.cpp TextureRenderer.h
	TestAppMain.h
)
set(FILES
	testcore/keybindings.cfg
)
engine_add_module(TARGET testcore FILES ${FILES} SRCS ${SRCS} DEPENDENCIES ui)
