set(SRCS
	TestApp.cpp TestApp.h
	TestMeshApp.cpp TestMeshApp.h
	TestAppMain.h
)
engine_add_module(TARGET testcore SRCS ${SRCS} DEPENDENCIES core frontend render video imgui)
