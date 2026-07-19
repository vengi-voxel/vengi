/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "video/TexturePool.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/ScenePaletteCache.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxelcollection/CollectionManager.h"
#include "core/Var.h"

namespace voxedit {
class MainWindow;
class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
} // namespace voxedit

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit : public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;

protected:
	voxedit::MainWindow *_mainWindow = nullptr;
	core::DynamicArray<io::FormatDescription> _paletteFormats;
	voxedit::SceneManagerPtr _sceneMgr;
	voxedit::SceneRendererPtr _sceneRenderer;
	voxelcollection::CollectionManagerPtr _collectionMgr;
	video::TexturePoolPtr _texturePool;
	voxedit::ScenePaletteCache _paletteCache;
	core::VarPtr _penPressureAffectsRadius;
	core::VarPtr _penRadiusMin;
	core::VarPtr _penRadiusMax;
	core::VarPtr _penEraserSwitchesMode;
	bool _penEraserActive = false;
	ModifierType _penPrevModifierType = ModifierType::Place;
	int _penBaseRadius = -1;

	// 0 is the default binding
	enum KeyBindings {
		Magicavoxel = 0,
		Blender = 1,
		Vengi = 2,
		Qubicle = 3,
		Goxel = 4,
		_3dsMax = 5,
		Max
	};

	void loadKeymap(int keymap) override;
	void validateKeyBindings() override;
	void importPalette(const core::String &file);

protected:
	void printUsageHeader() const override;

public:
	VoxEdit(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
			const voxedit::SceneManagerPtr &sceneMgr, const voxelcollection::CollectionManagerPtr &collectionMgr,
			const video::TexturePoolPtr &texturePool, const voxedit::SceneRendererPtr &sceneRenderer);

	void onRenderUI() override;

	void onDropFile(void *windowHandle, const core::String &file) override;
	void onDropText(void *windowHandle, const core::String& text) override;
	void onPenAxis(void *windowHandle, uint32_t pen, float x, float y, video::PenAxis axis, float value) override;
	void onPenDown(void *windowHandle, uint32_t pen, float x, float y, bool eraser) override;
	void onPenUp(void *windowHandle, uint32_t pen, float x, float y, bool eraser) override;
	bool allowedToQuit() override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;

	void toggleScene();
};
