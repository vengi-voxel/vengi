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
)
engine_add_library(
	LIB dearimgui
	SRCS ${SRCS}
)

target_include_directories(dearimgui PRIVATE dearimgui)