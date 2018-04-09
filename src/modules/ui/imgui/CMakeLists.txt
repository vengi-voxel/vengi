set(SRCS
	IMGUIApp.cpp IMGUIApp.h
	Console.cpp Console.h
	IMGUI.cpp IMGUI.h
	IMGUIInternal.h
	IMGUIAddons.h
)
set(LIB imgui)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core io render dearimgui)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC .)
