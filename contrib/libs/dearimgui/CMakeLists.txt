set(SRCS
	dearimgui/imgui.cpp dearimgui/imgui_draw.cpp
	dearimgui/imgui.h
	dearimgui/imgui_internal.h
	dearimgui/imconfig.h
	dearimgui/stb_rect_pack.h
	dearimgui/stb_textedit.h
	dearimgui/stb_truetype.h

	dearimgui/addons/imguinodegrapheditor/imguinodegrapheditor.h
	dearimgui/addons/imguinodegrapheditor/imguinodegrapheditor.cpp

	dearimgui/addons/imguihelper/imguihelper.h
	dearimgui/addons/imguihelper/imguihelper.cpp
)
engine_add_library(
	LIB dearimgui
	SRCS ${SRCS}
)

add_definitions(-DIMGUI_INCLUDE_IMGUI_USER_H -DIMGUI_INCLUDE_IMGUI_USER_INL)
target_include_directories(dearimgui PRIVATE dearimgui)