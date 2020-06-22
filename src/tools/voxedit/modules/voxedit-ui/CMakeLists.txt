set(LIB voxedit-ui)

set(SRCS
	editorscene/Viewport.h editorscene/Viewport.cpp

	palette/PaletteWidget.h palette/PaletteWidget.cpp
	palette/PaletteSelector.h palette/PaletteSelector.cpp

	settings/SceneSettingsWindow.h settings/SceneSettingsWindow.cpp

	layer/AbstractLayerPopupWindow.h layer/AbstractLayerPopupWindow.cpp
	layer/LayerWidget.h layer/LayerWidget.cpp
	layer/LayerWindow.h layer/LayerWindow.cpp
	layer/LayerMoveWindow.h
	layer/LayerRenameWindow.h

	VoxEditWindow.h VoxEditWindow.cpp
)

set(UI_WINDOWS
	voxedit-ui/ui/window/voxedit-layer-rename.tb.txt
	voxedit-ui/ui/window/voxedit-layer-move.tb.txt
	voxedit-ui/ui/window/voxedit-layer.tb.txt
	voxedit-ui/ui/window/voxedit-main.tb.txt
	voxedit-ui/ui/window/voxedit-palette-selector.tb.txt
	voxedit-ui/ui/window/voxedit-scene-settings.tb.txt
)

set(IMAGES
	voxedit-ui/ui/skin/voxedit-add
	voxedit-ui/ui/skin/voxedit-anim-play
	voxedit-ui/ui/skin/voxedit-anim-play-selected
	voxedit-ui/ui/skin/voxedit-animation
	voxedit-ui/ui/skin/voxedit-animation-selected
	voxedit-ui/ui/skin/voxedit-axis-x
	voxedit-ui/ui/skin/voxedit-axis-y
	voxedit-ui/ui/skin/voxedit-axis-z
	voxedit-ui/ui/skin/voxedit-color-picker
	voxedit-ui/ui/skin/voxedit-color
	voxedit-ui/ui/skin/voxedit-colors
	voxedit-ui/ui/skin/voxedit-crop
	voxedit-ui/ui/skin/voxedit-delete
	voxedit-ui/ui/skin/voxedit-export
	voxedit-ui/ui/skin/voxedit-extend
	voxedit-ui/ui/skin/voxedit-flip-horizontal
	voxedit-ui/ui/skin/voxedit-flip-vertical
	voxedit-ui/ui/skin/voxedit-import-palette
	voxedit-ui/ui/skin/voxedit-load
	voxedit-ui/ui/skin/voxedit-lock-layer
	voxedit-ui/ui/skin/voxedit-lock-layer-selected
	voxedit-ui/ui/skin/voxedit-move
	voxedit-ui/ui/skin/voxedit-new
	voxedit-ui/ui/skin/voxedit-options
	voxedit-ui/ui/skin/voxedit-override
	voxedit-ui/ui/skin/voxedit-quadview
	voxedit-ui/ui/skin/voxedit-quadview-selected
	voxedit-ui/ui/skin/voxedit-quit
	voxedit-ui/ui/skin/voxedit-undo-redo
	voxedit-ui/ui/skin/voxedit-reset-camera
	voxedit-ui/ui/skin/voxedit-rotate
	voxedit-ui/ui/skin/voxedit-save
	voxedit-ui/ui/skin/voxedit-shift
	voxedit-ui/ui/skin/voxedit-show-grid
	voxedit-ui/ui/skin/voxedit-show-grid-selected
	voxedit-ui/ui/skin/voxedit-tree_pine
	voxedit-ui/ui/skin/voxedit-tree
	voxedit-ui/ui/skin/voxedit-up
	voxedit-ui/ui/skin/voxedit-visible
	voxedit-ui/ui/skin/voxedit-visible-selected
	voxedit-ui/ui/skin/voxedit-voxel
)

set(FILES
	${UI_WINDOWS}
	voxedit-ui/ui/skin/voxedit-skin.tb.txt
	voxedit-ui/ui/widget/voxedit-layer-item.tb.txt
	voxedit-ui/ui/widget/voxedit-layer.tb.txt
	voxedit-ui/ui/lang/en.tb.txt
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

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES turbobadger voxedit-util)
check_ui_turbobadger(${LIB} ${UI_WINDOWS})
