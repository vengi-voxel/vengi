/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/Var.h"
#include "core/collection/ConcurrentQueue.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/VoxBoxApi.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

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
	SceneManagerPtr _sceneMgr;
	bool _requestPending = false;
	bool _open = false;
	bool _requestFocus = false;
	bool _showUpload = false;
	bool _useApiKey = false;

	void fetchModels();
	void loginPanel();
	void searchPanel(command::CommandExecutionListener *listener);
	void uploadPanel();

public:
	VoxBoxBrowserPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool);
	void open();
	void update(const char *id, command::CommandExecutionListener *listener = nullptr);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
