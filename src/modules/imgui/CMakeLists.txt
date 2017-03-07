set(SRCS
	IMGUIApp.cpp IMGUIApp.h

	imgui.cpp imgui_draw.cpp
)
set(LIB imgui)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} texture)
target_link_libraries(${LIB} core io video)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
