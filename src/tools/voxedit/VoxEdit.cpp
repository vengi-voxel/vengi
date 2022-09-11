/**
 * @file
 */

#include "VoxEdit.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "core/concurrent/Concurrency.h"
#include "io/FormatDescription.h"
#include "video/WindowedApp.h"
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
		Super(metric, filesystem, eventBus, timeProvider, core::halfcpus()) {
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
	core::Var::get(cfg::ClientFullscreen, "false");
	const app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);

	core::Var::get(cfg::VoxEditShowgrid, "1", "Show the grid", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditModelSpace, "1", "Model space", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowlockedaxis, "1", "Show the currently locked axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowaabb, "0", "Show the axis aligned bounding box", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditRendershadow, "1", "Render with shadows", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditAnimationSpeed, "100", "Millisecond delay between frames");
	core::Var::get(cfg::VoxEditGridsize, "1", "The size of the voxel grid", [](const core::String &val) {
		const int intVal = core::string::toInt(val);
		return intVal >= 1 && intVal <= 64;
	});
	core::Var::get(cfg::VoxEditLastFile, "");
	core::Var::get(cfg::VoxEditLastFiles, "");
	core::Var::get(cfg::VoxEditGrayInactive, "false", "Render the inactive layers in gray scale mode");
	core::Var::get(cfg::VoxEditHideInactive, "false", "Hide the inactive layers");
	core::Var::get(cfg::VoxEditModelSpace, "1");
	core::Var::get(cfg::VoxEditShowaxis, "1", "Show the axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoRotation, "0", "Activate rotations for the guizmo in scene mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoAllowAxisFlip, "1", "Flip axis or stay along the positive world/local axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoSnap, "1", "Use the grid size for snap", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditLastPalette, voxel::Palette::builtIn[0]);

	core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST, "Merge similar quads to optimize the mesh");
	core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST, "Reuse vertices or always create new ones");
	core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST, "Extra vertices for ambient occlusion");
	core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST, "Scale the vertices by the given factor");
	core::Var::get(cfg::VoxformatScaleX, "1.0", core::CV_NOPERSIST, "Scale the vertices on X axis by the given factor");
	core::Var::get(cfg::VoxformatScaleY, "1.0", core::CV_NOPERSIST, "Scale the vertices on Y axis by the given factor");
	core::Var::get(cfg::VoxformatScaleZ, "1.0", core::CV_NOPERSIST, "Scale the vertices on Z axis by the given factor");
	core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST, "Export as quads. If this false, triangles will be used.");
	core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST, "Export with vertex colors");
	core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST, "Export with uv coordinates of the palette image");
	core::Var::get(cfg::VoxformatTransform, "false", core::CV_NOPERSIST, "Apply the scene graph transform to mesh exports");
	core::Var::get(cfg::VoxformatFillHollow, "true", core::CV_NOPERSIST, "Fill the hollows when voxelizing a mesh format");
	core::Var::get(cfg::VoxelPalette, voxel::Palette::getDefaultPaletteName(), "This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)");

	static video::FileDialogOptions options = [] (video::OpenFileMode mode, const io::FormatDescription *desc) {
		if (mode == video::OpenFileMode::Directory) {
			return;
		}
		if (desc == nullptr) {
			return;
		}

		const bool forceApplyOptions = (desc->flags & FORMAT_FLAG_ALL) == FORMAT_FLAG_ALL;
		if (forceApplyOptions || voxelformat::isMeshFormat(*desc)) {
			ImGui::InputVarFloat("Uniform scale", cfg::VoxformatScale);
			ImGui::InputVarFloat("X axis scale", cfg::VoxformatScaleX);
			ImGui::InputVarFloat("Y axis scale", cfg::VoxformatScaleY);
			ImGui::InputVarFloat("Z axis scale", cfg::VoxformatScaleZ);

			if (mode == video::OpenFileMode::Save) {
				ImGui::CheckboxVar("Merge quads", cfg::VoxformatMergequads);
				ImGui::CheckboxVar("Reuse vertices", cfg::VoxformatReusevertices);
				ImGui::CheckboxVar("Ambient occlusion", cfg::VoxformatAmbientocclusion);
				ImGui::CheckboxVar("Apply transformations", cfg::VoxformatTransform);
				ImGui::CheckboxVar("Exports quads", cfg::VoxformatQuads);
				ImGui::CheckboxVar("Vertex colors", cfg::VoxformatWithcolor);
				ImGui::CheckboxVar("Texture coordinates", cfg::VoxformatWithtexcoords);
			} else if (mode == video::OpenFileMode::Open) {
				ImGui::CheckboxVar("Fill hollow", cfg::VoxformatFillHollow);
			}
		}
	};

	for (const io::FormatDescription* desc = io::format::palettes(); desc->valid(); ++desc) {
		_paletteFormats.push_back(*desc);
	}
	for (const io::FormatDescription* desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		if (desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED) {
			_paletteFormats.push_back(*desc);
		}
	}
	_paletteFormats.push_back(io::FormatDescription{"", {}, nullptr, 0u});

	voxedit::sceneMgr().construct();

	command::Command::registerCommand("screenshot", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = getSuggestedFilename("png");
			saveDialog([this] (const core::String &file) {_mainWindow->saveScreenshot(file); }, options, io::format::images(), filename);
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
				saveDialog([this] (const core::String &file) {_mainWindow->save(file); }, options, voxelformat::voxelSave());
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
		saveDialog([this] (const core::String &file) {_mainWindow->save(file); }, options, voxelformat::voxelSave(), filename);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("load", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file) {_mainWindow->load(file); }, options, voxelformat::voxelLoad());
			return;
		}
		_mainWindow->load(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Load a scene from the given volume file");

	command::Command::registerCommand("prefab", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([](const core::String &file) { voxedit::sceneMgr().prefab(file); }, options, voxelformat::voxelLoad());
			return;
		}
		voxedit::sceneMgr().prefab(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Add a volume to the existing scene from the given file");

	command::Command::registerCommand("importheightmap", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importHeightmap(file); }, options, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importHeightmap(args[0])) {
			Log::error("Failed to execute 'importheightmap' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active volume layer");

	command::Command::registerCommand("importcoloredheightmap", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importHeightmap(file); }, options, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importColoredHeightmap(args[0])) {
			Log::error("Failed to execute 'importcoloredheightmap' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active volume layer");

	command::Command::registerCommand("importplane", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importAsPlane(file); }, options, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importAsPlane(args[0])) {
			Log::error("Failed to execute 'importplane' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a plane into a new layer");

	command::Command::registerCommand("importvolume", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importAsVolume(file, 8, true); }, options, io::format::images());
			return;
		}
		const int maxDepth = args.size() >= 2 ? core::string::toInt(args[1]) : 8;
		const bool bothSides = args.size() >= 3 ? core::string::toBool(args[2]) : true;
		if (!voxedit::sceneMgr().importAsVolume(args[0], maxDepth, bothSides)) {
			Log::error("Failed to execute 'importvolume' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a volume into a new layer");

	command::Command::registerCommand("importpalette", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file) { voxedit::sceneMgr().importPalette(file); }, options, &_paletteFormats[0]);
			return;
		}
		if (!voxedit::sceneMgr().importPalette(args[0])) {
			Log::error("Failed to execute 'importpalette' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory, &_paletteFormats[0])).setHelp("Import an image as a palette");

#ifdef VOXEDIT_ANIMATION
	command::Command::registerCommand("animation_load", [&] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file) { _mainWindow->loadAnimationEntity(file); }, options, io::format::lua());
			return;
		}
		_mainWindow->loadAnimationEntity(args[0]);
	}).setHelp("Load the animation volumes and settings").setArgumentCompleter(command::fileCompleter(io::filesystem(), "", "*.lua"));
#endif

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

	if (_argc >= 2) {
		const char *file = _argv[_argc - 1];
		const io::FilePtr& filePtr = filesystem()->open(file);
		if (filePtr->exists()) {
			_mainWindow->load(file);
		}
	}

	return state;
}

void VoxEdit::onRenderUI() {
	if (voxedit::sceneMgr().update(_nowSeconds)) {
		_mainWindow->resetCamera();
	}
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
