/**
 * @file
 */

#include "VoxEdit.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "voxedit-util/Config.h"
#include "metric/Metric.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "video/Renderer.h"
#include "io/Filesystem.h"

#include "voxedit-util/SceneManager.h"
#include "voxedit-util/CustomBindingContext.h"

#include "voxedit-ui/MainWindow.h"
#include "voxelformat/VolumeFormat.h"

VoxEdit::VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "voxedit");
	_allowRelativeMouseMode = false;
}

app::AppState VoxEdit::onCleanup() {
	voxedit::sceneMgr().shutdown();
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
	return Super::onCleanup();
}

void VoxEdit::onDropFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return;
	}
	if (_mainWindow->isPaletteWidgetDropTarget()) {
		if (voxedit::sceneMgr().importPalette(file)) {
			return;
		}
	}
	if (voxedit::sceneMgr().prefab(file)) {
		return;
	}
	Log::warn("Failed to handle %s as drop file event", file.c_str());
}

core::String VoxEdit::getSuggestedFilename(const char *extension) const {
	core::String filename = voxedit::sceneMgr().filename();
	if (filename.empty()) {
		return filename;
	}
	if (extension == nullptr) {
		return filename;
	}
	return core::string::stripExtension(filename) + "." + extension;
}

app::AppState VoxEdit::onConstruct() {
	core::Var::get(cfg::ClientCameraMaxZoom, "1000.0");
	core::Var::get(cfg::ClientCameraMinZoom, "0.1");
	const app::AppState state = Super::onConstruct();

	_framesPerSecondsCap->setVal(60.0f);

	_paletteFormats[0] = {"Image", "png", nullptr, 0u};
	int formatIndex = 1;
	for (const io::FormatDescription* desc = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD; desc->name != nullptr; ++desc) {
		if (desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED) {
			_paletteFormats[formatIndex++] = *desc;
		}
	}
	_paletteFormats[formatIndex++] = {nullptr, nullptr, nullptr, 0u};

	voxedit::sceneMgr().construct();

	command::Command::registerCommand("screenshot", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = getSuggestedFilename("png");
			saveDialog([this] (const core::String &file) {_mainWindow->saveScreenshot(file); }, io::format::images(), filename);
			return;
		}
		_mainWindow->saveScreenshot(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current viewport as screenshot");

	command::Command::registerCommand("save", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = getSuggestedFilename();
			if (filename.empty()) {
				saveDialog([this] (const core::String &file) {_mainWindow->save(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE);
			} else {
				_mainWindow->save(filename);
			}
			return;
		}
		_mainWindow->save(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("saveas", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		const core::String filename = getSuggestedFilename();
		saveDialog([this] (const core::String &file) {_mainWindow->save(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE, filename);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("load", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file) {_mainWindow->load(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
			return;
		}
		_mainWindow->load(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Load a scene from the given volume file");

	command::Command::registerCommand("prefab", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([](const core::String &file) { voxedit::sceneMgr().prefab(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
			return;
		}
		voxedit::sceneMgr().prefab(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Add a volume to the existing scene from the given file");

	command::Command::registerCommand("importheightmap", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importHeightmap(file); }, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importHeightmap(args[0])) {
			Log::error("Failed to execute 'importheightmap' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active volume layer");

	command::Command::registerCommand("importplane", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importAsPlane(file); }, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importAsPlane(args[0])) {
			Log::error("Failed to execute 'importplane' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a plane into a new layer");

	command::Command::registerCommand("importpalette", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importPalette(file); }, &_paletteFormats[0]);
			return;
		}
		if (!voxedit::sceneMgr().importPalette(args[0])) {
			Log::error("Failed to execute 'importpalette' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a palette");

	command::Command::registerCommand("animation_load", [&] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file) { _mainWindow->loadAnimationEntity(file); }, io::format::lua());
			return;
		}
		_mainWindow->loadAnimationEntity(args[0]);
	}).setHelp("Load the animation volumes and settings").setArgumentCompleter(command::fileCompleter(io::filesystem(), "", "*.lua"));

	command::Command::registerCommand("new", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return false;
		}
		return _mainWindow->createNew(false);
	}).setHelp("Create a new scene with ui interaction");

	command::Command::registerCommand("resetcamera", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->resetCamera();
	}).setHelp("Reset cameras in all viewports");

	return state;
}

app::AppState VoxEdit::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!voxedit::sceneMgr().init()) {
		Log::error("Failed to initialize the scene manager");
		return app::AppState::InitFailure;
	}

	if (_argc >= 2) {
		const char *file = _argv[_argc - 1];
		const io::FilePtr& filePtr = filesystem()->open(file);
		if (filePtr->exists()) {
			core::Var::get(cfg::VoxEditLastFile)->setVal(filePtr->name());
		}
	}

	_mainWindow = new voxedit::MainWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	// needed for handling the module includes
	if (!filesystem()->registerPath("scripts/")) {
		Log::error("Failed to register lua generator script path");
		return app::AppState::InitFailure;
	}

	video::clearColor(::core::Color::Black);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	setRelativeMouseMode(false);

	core::setBindingContext(voxedit::BindingContext::UI);

	return state;
}

void VoxEdit::onRenderUI() {
	voxedit::sceneMgr().update(_nowSeconds);
	_mainWindow->update();
}

app::AppState VoxEdit::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	const bool isSceneHovered = _mainWindow->isSceneHovered();
	if (isSceneHovered) {
		if (voxedit::sceneMgr().editMode() == voxedit::EditMode::Scene) {
			core::setBindingContext(voxedit::BindingContext::Scene);
		} else {
			core::setBindingContext(voxedit::BindingContext::Model);
		}
	} else {
		core::setBindingContext(voxedit::BindingContext::UI);
	}
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxEdit app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
