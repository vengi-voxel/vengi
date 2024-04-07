set(LIB ui)

set(SRCS
	FileDialog.cpp FileDialog.h
	IconsLucide.h
	IMGUIConsole.cpp IMGUIConsole.h
	IMGUIEx.cpp IMGUIEx.h
	IMGUIApp.cpp IMGUIApp.h
	IMGUIStyle.cpp IMGUIStyle.h
	Notify.cpp Notify.h
	Panel.h
	PopupAbout.h PopupAbout.cpp
	ScopedStyle.h
	Style.cpp Style.h
	TextEditor.h TextEditor.cpp
	Toolbar.h Toolbar.cpp

	dearimgui/imgui.cpp dearimgui/imgui_draw.cpp
	dearimgui/imgui_widgets.cpp
	dearimgui/imgui_tables.cpp
	dearimgui/imgui_stdlib.cpp dearimgui/imgui_stdlib.h
	dearimgui/imgui.h
	dearimgui/imgui_internal.h
	dearimgui/imconfig.h
	dearimgui/imstb_rectpack.h
	dearimgui/imstb_textedit.h
	dearimgui/imstb_truetype.h

	dearimgui/backends/imgui_impl_opengl3.cpp dearimgui/backends/imgui_impl_opengl3.h
	dearimgui/backends/imgui_impl_sdl2.cpp dearimgui/backends/imgui_impl_sdl2.h

	dearimgui/imgui_neo_sequencer.cpp dearimgui/imgui_neo_sequencer.h
	dearimgui/imgui_neo_internal.cpp dearimgui/imgui_neo_internal.h

	dearimgui/implot.cpp dearimgui/implot.h
	dearimgui/implot_items.cpp dearimgui/implot_internal.h

	dearimgui/ImGuizmo.cpp dearimgui/ImGuizmo.h
)

set(DEFINITIONS -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -DIMGUI_DISABLE_OBSOLETE_KEYIO)
set(LIBS)

if (USE_OPENGLES)
	list(APPEND DEFINITIONS -DIMGUI_IMPL_OPENGL_ES2)
endif()

find_package(Freetype)
if (FREETYPE_FOUND)
	list(APPEND SRCS dearimgui/misc/freetype/imgui_freetype.cpp)
	list(APPEND DEFINITIONS -DIMGUI_ENABLE_FREETYPE)
	list(APPEND LIBS Freetype::Freetype)
endif()

macro(imgui_build_font TARGET NAME TTF_FILE)
	file(MAKE_DIRECTORY ${GENERATE_DIR}/${TARGET})
	engine_compressed_file_to_header(${TARGET} ${NAME} ${ROOT_DIR}/data/${TARGET}/${TTF_FILE} ${GENERATE_DIR}/${TARGET}/${NAME}.h)
endmacro()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES render app ${LIBS})
target_compile_definitions(${LIB} PUBLIC ${DEFINITIONS})

imgui_build_font(${LIB} FontLucide lucide.ttf)
imgui_build_font(${LIB} ArimoRegular Arimo-Regular.ttf)

target_include_directories(${LIB} PUBLIC . ${GENERATE_DIR}/${LIB} dearimgui)
set_target_properties(${LIB} PROPERTIES UNITY_BUILD OFF)
