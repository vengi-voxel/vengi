add_subdirectory(voxedit-util)

project(voxedit)
set(SRCS
	ui/editorscene/Viewport.h ui/editorscene/Viewport.cpp

	ui/palette/PaletteWidget.h ui/palette/PaletteWidget.cpp
	ui/palette/PaletteSelector.h ui/palette/PaletteSelector.cpp

	ui/settings/SceneSettingsWindow.h ui/settings/SceneSettingsWindow.cpp

	ui/layer/AbstractLayerPopupWindow.h ui/layer/AbstractLayerPopupWindow.cpp
	ui/layer/LayerWidget.h ui/layer/LayerWidget.cpp
	ui/layer/LayerWindow.h ui/layer/LayerWindow.cpp
	ui/layer/LayerMoveWindow.h
	ui/layer/LayerRenameWindow.h

	ui/VoxEditWindow.h ui/VoxEditWindow.cpp

	VoxEdit.h VoxEdit.cpp
)

set(UI_WINDOWS
	voxedit/ui/window/voxedit-layer-rename.tb.txt
	voxedit/ui/window/voxedit-layer-move.tb.txt
	voxedit/ui/window/voxedit-layer.tb.txt
	voxedit/ui/window/voxedit-main.tb.txt
	voxedit/ui/window/voxedit-palette-selector.tb.txt
	voxedit/ui/window/voxedit-scene-settings.tb.txt
)

set(IMAGES
	voxedit/ui/skin/voxedit-add
	voxedit/ui/skin/voxedit-anim-play
	voxedit/ui/skin/voxedit-anim-play-selected
	voxedit/ui/skin/voxedit-animation
	voxedit/ui/skin/voxedit-animation-selected
	voxedit/ui/skin/voxedit-axis-x
	voxedit/ui/skin/voxedit-axis-y
	voxedit/ui/skin/voxedit-axis-z
	voxedit/ui/skin/voxedit-color-picker
	voxedit/ui/skin/voxedit-color
	voxedit/ui/skin/voxedit-colors
	voxedit/ui/skin/voxedit-crop
	voxedit/ui/skin/voxedit-delete
	voxedit/ui/skin/voxedit-export
	voxedit/ui/skin/voxedit-extend
	voxedit/ui/skin/voxedit-flip-horizontal
	voxedit/ui/skin/voxedit-flip-vertical
	voxedit/ui/skin/voxedit-import-palette
	voxedit/ui/skin/voxedit-load
	voxedit/ui/skin/voxedit-lock-layer
	voxedit/ui/skin/voxedit-lock-layer-selected
	voxedit/ui/skin/voxedit-move
	voxedit/ui/skin/voxedit-new
	voxedit/ui/skin/voxedit-options
	voxedit/ui/skin/voxedit-override
	voxedit/ui/skin/voxedit-quadview
	voxedit/ui/skin/voxedit-quadview-selected
	voxedit/ui/skin/voxedit-quit
	voxedit/ui/skin/voxedit-undo-redo
	voxedit/ui/skin/voxedit-reset-camera
	voxedit/ui/skin/voxedit-rotate
	voxedit/ui/skin/voxedit-save
	voxedit/ui/skin/voxedit-shift
	voxedit/ui/skin/voxedit-show-grid
	voxedit/ui/skin/voxedit-show-grid-selected
	voxedit/ui/skin/voxedit-tree_pine
	voxedit/ui/skin/voxedit-tree
	voxedit/ui/skin/voxedit-up
	voxedit/ui/skin/voxedit-visible
	voxedit/ui/skin/voxedit-visible-selected
	voxedit/ui/skin/voxedit-voxel
)

set(FILES
	${UI_WINDOWS}
	voxedit/ui/skin/voxedit-skin.tb.txt
	voxedit/ui/widget/voxedit-layer-item.tb.txt
	voxedit/ui/widget/voxedit-layer.tb.txt
	voxedit/ui/lang/en.tb.txt

	voxedit/voxedit-keybindings.cfg

	voxedit/scripts/test-import-plane.cfg
	voxedit/scripts/test-layers.cfg
	voxedit/scripts/test-animate.cfg
)

foreach(IMAGE ${IMAGES})
	list(APPEND FILES ${IMAGE}.png)
endforeach()
set(DPIS "@288")
foreach(DPI ${DPIS})
	foreach(IMAGE ${IMAGES})
		list(APPEND FILES ${IMAGE}${DPI}.png)
	endforeach()
endforeach()

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger voxedit-util)
include_directories(.)
check_ui_turbobadger(${PROJECT_NAME} ${UI_WINDOWS})
