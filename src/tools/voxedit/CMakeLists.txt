add_subdirectory(voxedit-util)

project(voxedit)
set(SRCS
	ui/editorscene/Viewport.h ui/editorscene/Viewport.cpp
	ui/editorscene/Controller.h ui/editorscene/Controller.cpp

	ui/palette/PaletteWidget.h ui/palette/PaletteWidget.cpp

	ui/layer/LayerWidget.h ui/layer/LayerWidget.cpp
	ui/layer/LayerWindow.h ui/layer/LayerWindow.cpp

	ui/VoxEditWindow.h ui/VoxEditWindow.cpp
	ui/TreeWindow.h ui/TreeWindow.cpp
	ui/NoiseWindow.h ui/NoiseWindow.cpp

	VoxEdit.h VoxEdit.cpp
)

set(FILES
	voxedit/ui/skin/voxedit-axis-x.png
	voxedit/ui/skin/voxedit-axis-y.png
	voxedit/ui/skin/voxedit-axis-z.png
	voxedit/ui/skin/voxedit-color-picker.png
	voxedit/ui/skin/voxedit-color.png
	voxedit/ui/skin/voxedit-crop.png
	voxedit/ui/skin/voxedit-delete.png
	voxedit/ui/skin/voxedit-export.png
	voxedit/ui/skin/voxedit-extend.png
	voxedit/ui/skin/voxedit-flip-horizontal.png
	voxedit/ui/skin/voxedit-flip-vertical.png
	voxedit/ui/skin/voxedit-load.png
	voxedit/ui/skin/voxedit-new.png
	voxedit/ui/skin/voxedit-options.png
	voxedit/ui/skin/voxedit-override.png
	voxedit/ui/skin/voxedit-quadview.png
	voxedit/ui/skin/voxedit-quadview-selected.png
	voxedit/ui/skin/voxedit-quit.png
	voxedit/ui/skin/voxedit-redo.png
	voxedit/ui/skin/voxedit-reset-camera.png
	voxedit/ui/skin/voxedit-rotate-left.png
	voxedit/ui/skin/voxedit-rotate-right.png
	voxedit/ui/skin/voxedit-save.png
	voxedit/ui/skin/voxedit-show-grid.png
	voxedit/ui/skin/voxedit-show-grid-selected.png
	voxedit/ui/skin/voxedit-skin.tb.txt
	voxedit/ui/skin/voxedit-tree_pine.png
	voxedit/ui/skin/voxedit-tree.png
	voxedit/ui/skin/voxedit-undo.png
	voxedit/ui/skin/voxedit-visible.png
	voxedit/ui/skin/voxedit-visible-selected.png
	voxedit/ui/skin/voxedit-voxel.png
	voxedit/ui/widget/voxedit-layer-item.tb.txt
	voxedit/ui/widget/voxedit-layer.tb.txt
	voxedit/ui/window/voxedit-layer.tb.txt
	voxedit/ui/window/voxedit-main.tb.txt
	voxedit/ui/window/voxedit-noise.tb.txt
	voxedit/ui/window/voxedit-tree.tb.txt
	voxedit/ui/lang/en.tb.txt
	voxedit/models/armchair.vox
	voxedit/models/bowl_bone.vox
	voxedit/models/brick_wall.vox
	voxedit/models/carter.vox
	voxedit/models/cart_full.vox
	voxedit/models/cart.vox
	voxedit/models/cat_01.vox
	voxedit/models/cat_02.vox
	voxedit/models/cat_03.vox
	voxedit/models/chimney.vox
	voxedit/models/christmas_scene.vox
	voxedit/models/christmas_tree.vox
	voxedit/models/couch.vox
	voxedit/models/dog_house.vox
	voxedit/models/dog_scene.vox
	voxedit/models/dog.vox
	voxedit/models/door.vox
	voxedit/models/gifts.vox
	voxedit/models/horse.vox
	voxedit/models/lantern.vox
	voxedit/models/mailbox.vox
	voxedit/models/man.vox
	voxedit/models/odyssey_scene.vox
	voxedit/models/paperboy.vox
	voxedit/models/README.md
	voxedit/models/santa.vox
	voxedit/models/snowman.vox
	voxedit/models/spaceship_interior.vox
	voxedit/models/spacesuit.vox
	voxedit/models/street_scene.vox
	voxedit/models/street.vox
	voxedit/models/treehouse.vox
	voxedit/models/woman.vox
	voxedit/models/chr_knight.qb
	voxedit/models/chr_knight.qbt

	voxedit/voxedit-keybindings.cfg
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger voxedit-util)
include_directories(.)
check_ui_turbobadger(${PROJECT_NAME})
