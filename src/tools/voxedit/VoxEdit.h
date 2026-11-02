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
#include "voxelui/ScenePreview.h"
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
	voxelui::ScenePreview _fileDialogPreview;
	core::VarPtr _penPressureAffectsRadius;
	core::VarPtr _penRadiusMin;
	core::VarPtr _penRadiusMax;
	core::VarPtr _penEraserSwitchesMode;
	core::VarPtr _emulatePen;
	bool _penEraserActive = false;
	bool _emulatedPenDown = false;
	bool _emulatedPenMoved = false;
	int _emulatedPenTravel = 0;
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
	bool emulatePenActive(int32_t mouseId) const;
	static float emulatedPenPressure(int relX, int relY);
	void endEmulatedPen(void *windowHandle, int32_t x, int32_t y);

protected:
	void printUsageHeader() const override;

public:
	VoxEdit(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
			const voxedit::SceneManagerPtr &sceneMgr, const voxelcollection::CollectionManagerPtr &collectionMgr,
			const video::TexturePoolPtr &texturePool, const voxedit::SceneRendererPtr &sceneRenderer);

	void onRenderUI() override;
	bool onMouseWheel(void *windowHandle, float x, float y, int32_t mouseId) override;
	void onMouseButtonPress(void *windowHandle, int32_t x, int32_t y, uint8_t button, uint8_t clicks, int32_t mouseId) override;
	void onMouseButtonRelease(void *windowHandle, int32_t x, int32_t y, uint8_t button, int32_t mouseId) override;
	void onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY, int32_t mouseId) override;
	bool onFingerPress(void *windowHandle, int64_t finger, float x, float y, float pressure, uint32_t timestamp) override;
	void onFingerMotion(void *windowHandle, int64_t finger, float x, float y, float dx, float dy, float pressure,
						uint32_t timestamp) override;

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
