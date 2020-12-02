set(LIB voxedit-imgui)

set(SRCS
	VoxEditWindow.h VoxEditWindow.cpp
	Viewport.h Viewport.cpp
)

set(IMAGES
	voxedit-ui/ui/skin/voxedit-add.png
	voxedit-ui/ui/skin/voxedit-anim-play.png
	voxedit-ui/ui/skin/voxedit-anim-play-selected.png
	voxedit-ui/ui/skin/voxedit-animation.png
	voxedit-ui/ui/skin/voxedit-animation-selected.png
	voxedit-ui/ui/skin/voxedit-axis-x.png
	voxedit-ui/ui/skin/voxedit-axis-y.png
	voxedit-ui/ui/skin/voxedit-axis-z.png
	voxedit-ui/ui/skin/voxedit-color-picker.png
	voxedit-ui/ui/skin/voxedit-color.png
	voxedit-ui/ui/skin/voxedit-colors.png
	voxedit-ui/ui/skin/voxedit-crop.png
	voxedit-ui/ui/skin/voxedit-delete.png
	voxedit-ui/ui/skin/voxedit-export.png
	voxedit-ui/ui/skin/voxedit-extend.png
	voxedit-ui/ui/skin/voxedit-flip-horizontal.png
	voxedit-ui/ui/skin/voxedit-flip-vertical.png
	voxedit-ui/ui/skin/voxedit-import-palette.png
	voxedit-ui/ui/skin/voxedit-load.png
	voxedit-ui/ui/skin/voxedit-lock-layer.png
	voxedit-ui/ui/skin/voxedit-lock-layer-selected.png
	voxedit-ui/ui/skin/voxedit-move.png
	voxedit-ui/ui/skin/voxedit-new.png
	voxedit-ui/ui/skin/voxedit-options.png
	voxedit-ui/ui/skin/voxedit-override.png
	voxedit-ui/ui/skin/voxedit-quadview.png
	voxedit-ui/ui/skin/voxedit-quadview-selected.png
	voxedit-ui/ui/skin/voxedit-quit.png
	voxedit-ui/ui/skin/voxedit-undo-redo.png
	voxedit-ui/ui/skin/voxedit-reset-camera.png
	voxedit-ui/ui/skin/voxedit-rotate.png
	voxedit-ui/ui/skin/voxedit-save.png
	voxedit-ui/ui/skin/voxedit-shift.png
	voxedit-ui/ui/skin/voxedit-show-grid.png
	voxedit-ui/ui/skin/voxedit-show-grid-selected.png
	voxedit-ui/ui/skin/voxedit-tree_pine.png
	voxedit-ui/ui/skin/voxedit-tree.png
	voxedit-ui/ui/skin/voxedit-up.png
	voxedit-ui/ui/skin/voxedit-visible.png
	voxedit-ui/ui/skin/voxedit-visible-selected.png
	voxedit-ui/ui/skin/voxedit-voxel.png
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES imgui voxedit-util)
