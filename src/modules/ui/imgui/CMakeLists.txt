set(SRCS
	IMGUIApp.cpp IMGUIApp.h
	Console.cpp Console.h
	IMGUI.cpp IMGUI.h
	IMGUIInternal.h
	IMGUIAddons.h
)
set(LIB imgui)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES render dearimgui)
target_include_directories(${LIB} PUBLIC .)
