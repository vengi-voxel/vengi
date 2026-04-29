/**
 * @file
 */

#pragma once

#include "core/Function.h"
#include "core/SharedPtr.h"
#include "core/Var.h"
#include "core/collection/ConcurrentQueue.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxelui/VoxBoxApi.h"

namespace command {
struct CommandExecutionListener;
}

namespace scenegraph {
class SceneGraph;
}

namespace voxelui {

/**
 * @brief Callback to load a .vengi file as a new scene (replaces current).
 * @param path Full filesystem path to the .vengi file.
 */
using VoxBoxLoadFunc = core::Function<bool(const core::String &path)>;

/**
 * @brief Callback to import a .vengi file into the current scene.
 * @param path Full filesystem path to the .vengi file.
 */
using VoxBoxImportFunc = core::Function<bool(const core::String &path)>;

/**
 * @brief Callback to get a mutable reference to the current scene graph.
 */
using VoxBoxSceneGraphFunc = core::Function<scenegraph::SceneGraph &()>;

class VoxBoxBrowserPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	VoxBoxApi _api;
	VoxBoxState _state;
	VoxBoxSearchParams _searchParams;
	VoxBoxModelInfo _uploadInfo;
	video::TexturePoolPtr _texturePool;
	io::ArchivePtr _archive;
	core::SharedPtr<core::ConcurrentQueue<image::ImagePtr>> _imageQueue;
	core::VarPtr _varUsername;
	core::VarPtr _varPassword;
	core::VarPtr _varApiKey;
	core::String _loginError;
	core::String _uploadCoverFile;
	VoxBoxLoadFunc _loadFunc;
	VoxBoxImportFunc _importFunc;
	VoxBoxSceneGraphFunc _sceneGraphFunc;
	bool _requestPending = false;
	bool _open = false;
	bool _requestFocus = false;
	bool _showUpload = false;

	void fetchModels();
	void loginPanel();
	void searchPanel(command::CommandExecutionListener *listener);
	void uploadPanel();

public:
	VoxBoxBrowserPanel(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool);
	void setCallbacks(VoxBoxLoadFunc &&load, VoxBoxImportFunc &&import, VoxBoxSceneGraphFunc &&sceneGraph);
	void open();
	void update(const char *id, command::CommandExecutionListener *listener = nullptr);
};

} // namespace voxelui
