set(LIB ui)

set(SRCS
	Console.cpp Console.h
	FileDialog.cpp FileDialog.h
	IconsFontAwesome6.h
	IconsForkAwesome.h
	IMGUIEx.cpp IMGUIEx.h
	IMGUIApp.cpp IMGUIApp.h
	IMGUIStyle.cpp IMGUIStyle.h
	Notify.cpp Notify.h
	ScopedStyle.h
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

	dearimgui/backends/imgui_impl_sdl.cpp dearimgui/backends/imgui_impl_sdl.h

	dearimgui/imgui_neo_sequencer.cpp dearimgui/imgui_neo_sequencer.h
	dearimgui/imgui_neo_internal.cpp dearimgui/imgui_neo_internal.h

	dearimgui/ImGuizmo.cpp dearimgui/ImGuizmo.h
)

set(GEN_DIR ${GENERATE_DIR}/${LIB})
file(MAKE_DIRECTORY ${GEN_DIR})

macro(imgui_build_font NAME TTF_FILE)
	set(OUTPUT_FILE ${GEN_DIR}/${NAME}.h)
	set(INPUT_FILE ${ROOT_DIR}/data/${LIB}/${TTF_FILE})
	add_custom_command(
		OUTPUT ${OUTPUT_FILE}
		COMMAND
			binary_to_compressed_c
			${INPUT_FILE}
			${NAME} > ${OUTPUT_FILE}
		DEPENDS ${INPUT_FILE} binary_to_compressed_c
		WORKING_DIRECTORY ${GEN_DIR}
		COMMENT "Generate header for ${TTF_FILE} in ${GEN_DIR}"
		VERBATIM
	)
	list(APPEND SRCS ${OUTPUT_FILE})
	engine_mark_as_generated(${OUTPUT_FILE})
endmacro()

imgui_build_font(FontAwesomeSolid fa-solid-900.ttf)
imgui_build_font(ForkAwesomeWebFont forkawesome-webfont.ttf)
imgui_build_font(ArimoRegular Arimo-Regular.ttf)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES render app)
target_compile_definitions(${LIB} PUBLIC -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -DIMGUI_DISABLE_OBSOLETE_KEYIO)
target_include_directories(${LIB} PUBLIC . ${GEN_DIR} dearimgui)
set_target_properties(${LIB} PROPERTIES UNITY_BUILD OFF)
