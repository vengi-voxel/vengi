set(SRCS
	IMGUIApp.cpp IMGUIApp.h

	imgui.cpp imgui_draw.cpp
	imgui.h
	imgui_internal.h
	imconfig.h
	stb_rect_pack.h
	stb_textedit.h
	stb_truetype.h

	addons/imguinodegrapheditor/imguinodegrapheditor.h
	addons/imguinodegrapheditor/imguinodegrapheditor.cpp
)
set(LIB imgui)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} texture)
target_link_libraries(${LIB} core io video)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC .)
