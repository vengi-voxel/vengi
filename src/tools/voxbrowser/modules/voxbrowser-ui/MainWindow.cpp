/**
 * @file
 */

#include "MainWindow.h"
#include "IMGUIEx.h"
#include "ScopedStyle.h"
#include "app/App.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "image/Image.h"
#include "imgui.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "voxbrowser-util/Downloader.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"

#define TITLE_STATUSBAR "##statusbar"
#define TITLE_ASSET "Asset##asset"
#define TITLE_ASSET_DETAILS "Details##asset-details"
#define TITLE_ASSET_LIST "Assets##list"

namespace voxbrowser {

MainWindow::MainWindow(ui::IMGUIApp *app) : _app(app) {
}

MainWindow::~MainWindow() {
}

bool MainWindow::filtered(const VoxelFile &voxelFile) const {
	if (_currentFilterEntry <= 0) {
		return false;
	}
	const core::String &filter = _filterEntries[_currentFilterEntry].wildCard();
	const core::String &ext = core::string::extractExtension(voxelFile.name);
	if (core::string::fileMatchesMultiple(voxelFile.name.c_str(), filter.c_str())) {
		return false;
	}
	return true;
}

void MainWindow::updateFilters() {
	if (_filterTextWidth < 0.0f) {
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			_filterEntries.push_back(*desc);
			const core::String &str = io::convertToFilePattern(*desc);
			const ImVec2 filterTextSize = ImGui::CalcTextSize(str.c_str());
			_filterTextWidth = core_max(_filterTextWidth, filterTextSize.x);
		}
		_filterEntries.sort(core::Greater<io::FormatDescription>());
		io::createGroupPatterns(voxelformat::voxelLoad(), _filterEntries);
		// must be the first entry - see applyFilter()
		_filterEntries.insert(_filterEntries.begin(), io::ALL_SUPPORTED());
	}

	const char *label = "Filter";
	ImGui::PushItemWidth(_filterTextWidth);
	int currentlySelected = _currentFilterEntry == -1 ? 0 : _currentFilterEntry;
	const core::String &selectedEntry = io::convertToFilePattern(_filterEntries[currentlySelected]);

	if (ImGui::BeginCombo(label, selectedEntry.c_str(), ImGuiComboFlags_HeightLargest)) {
		for (int i = 0; i < (int)_filterEntries.size(); ++i) {
			const bool selected = i == currentlySelected;
			const io::FormatDescription &format = _filterEntries[i];
			const core::String &text = io::convertToFilePattern(format);
			if (ImGui::Selectable(text.c_str(), selected)) {
				_currentFilterEntry = i;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
}

bool MainWindow::downloadIfNeeded(const VoxelFile &voxelFile) {
	const core::String &relTargetFile = voxelFile.targetFile();
	if (!io::filesystem()->exists(relTargetFile)) {
		const core::String &relTargetPath = voxelFile.targetDir();
		if (!io::filesystem()->createDir(core::string::path(io::filesystem()->homePath(), relTargetPath))) {
			Log::error("Failed to create directory %s", relTargetPath.c_str());
			return false;
		}
		const io::FilePtr &filePtr = io::filesystem()->open(relTargetFile, io::FileMode::Write);
		io::FileStream stream(filePtr);
		if (!http::download(voxelFile.url, stream)) {
			Log::error("Failed to download %s", voxelFile.url.c_str());
			return false;
		}
	}
	return true;
}

bool MainWindow::downloadThumbnailIfNeeded(VoxelFile &voxelFile) {
	const core::String &relTargetFile = voxelFile.targetFile();
	const core::String &relImageFile = relTargetFile + ".png";
	if (io::filesystem()->exists(relImageFile)) {
		voxelFile.thumbnailImage = image::loadImage(relImageFile);
		return true;
	}

	if (!voxelFile.thumbnailUrl.empty()) {
		const core::String &relTargetPath = voxelFile.targetDir();
		if (!io::filesystem()->createDir(core::string::path(io::filesystem()->homePath(), voxelFile.targetDir()))) {
			Log::error("Failed to create directory %s", relTargetPath.c_str());
			return false;
		}
		const io::FilePtr &filePtr = io::filesystem()->open(relImageFile, io::FileMode::Write);
		io::FileStream stream(filePtr);
		if (!http::download(voxelFile.thumbnailUrl, stream)) {
			Log::warn("Failed to download thumbnail %s", voxelFile.thumbnailUrl.c_str());
			return false;
		}
		voxelFile.thumbnailImage = image::loadImage(relImageFile);
		return true;
	}
	return false;
}

bool MainWindow::createThumbnailIfNeeded(VoxelFile &voxelFile) {
	if (voxelFile.thumbnailImage) {
		return true;
	}
	voxelformat::LoadContext loadCtx;
	const core::String &relTargetFile = voxelFile.targetFile();
	const core::String &relImageFile = relTargetFile + ".png";
	const io::FilePtr &file = io::filesystem()->open(relTargetFile, io::FileMode::Read);
	io::FileStream stream(file);
	voxelFile.thumbnailImage = voxelformat::loadScreenshot(relTargetFile, stream, loadCtx);
	if (!voxelFile.thumbnailImage) {
		Log::warn("Use the thumbnail renderer");
		scenegraph::SceneGraph sceneGraph;
		stream.seek(0);
		io::FileDescription fileDesc;
		fileDesc.set(relTargetFile);
		if (!voxelformat::loadFormat(fileDesc, stream, sceneGraph, loadCtx)) {
			Log::error("Failed to load given input file: %s", relTargetFile.c_str());
			voxelFile.thumbnailImage = image::createEmptyImage(voxelFile.name);
			return false;
		}

		voxelFile.thumbnailImage = voxelrender::volumeThumbnail(sceneGraph, _thumbnailCtx);
	}
	voxelFile.thumbnailImage->setName(voxelFile.name);
	const core::String targetImagePath = io::filesystem()->writePath(relImageFile.c_str());
	if (!image::writeImage(voxelFile.thumbnailImage, targetImagePath)) {
		Log::warn("Failed to save thumbnail to %s", targetImagePath.c_str());
	} else {
		_texturePool.addImage(voxelFile.thumbnailImage);
		_texturePool.load(voxelFile.thumbnailImage->name());
	}
	return true;
}

void MainWindow::updateAsset() {
	VoxelFile &voxelFile = _selected;
	if (!voxelFile.thumbnailImage) {
		const core::String &relTargetFile = voxelFile.targetFile();
		const core::String &relImageFile = relTargetFile + ".png";
		downloadThumbnailIfNeeded(voxelFile);
		downloadIfNeeded(voxelFile);
		createThumbnailIfNeeded(voxelFile);
	}

	if (ImGui::Begin(TITLE_ASSET)) {
		if (voxelFile.thumbnailImage) {
			if (const video::TexturePtr &texture = _texturePool.get(voxelFile.thumbnailImage->name())) {
				ImGui::Image(texture->handle(), _thumbnailCtx.outputSize);
			}
		} else {
			ImGui::Text("No thumbnail available");
		}
	}
	ImGui::End();
}

void MainWindow::updateAssetDetails() {
	const VoxelFile &voxelFile = _selected;
	if (ImGui::Begin(TITLE_ASSET_DETAILS)) {
		ImGui::Text("Name: %s", voxelFile.name.c_str());
		ImGui::Text("Source: %s", voxelFile.source.c_str());
		ImGui::Text("License: %s", voxelFile.license.c_str());
		ImGui::URLItem(voxelFile.url.c_str(), voxelFile.url.c_str());
	}
	ImGui::End();
}

void MainWindow::updateAssetList(const voxbrowser::VoxelFileMap &voxelFilesMap) {
	if (ImGui::Begin(TITLE_ASSET_LIST)) {
		updateFilters();

		if (ImGui::TreeNode("Voxel Files", "Voxel Files (%d)", (int)voxelFilesMap.size())) {
			for (const auto &entry : voxelFilesMap) {
				if (ImGui::TreeNode(entry->first.c_str())) {
					const VoxelFiles &voxelFiles = entry->second;
					for (const VoxelFile &voxelFile : voxelFiles) {
						if (filtered(voxelFile)) {
							continue;
						}

						const bool selected = _selected == voxelFile;
						if (ImGui::Selectable(voxelFile.name.c_str(), selected,
											  ImGuiSelectableFlags_AllowDoubleClick)) {
							_selected = voxelFile;
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

void MainWindow::configureLeftTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_ASSET_LIST, dockId);
}

void MainWindow::configureMainTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_ASSET, dockId);
}

void MainWindow::configureMainBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_ASSET_DETAILS, dockId);
	ImGui::DockBuilderDockWindow(UI_CONSOLE_WINDOW_TITLE, dockId);
}

void MainWindow::update(const voxbrowser::VoxelFileMap &voxelFilesMap) {
	// TODO: download images and create image::ImagePtr instance (and cache them on disc)
	// TODO: allow to download voxel files
	// TODO: allow to specify target directory and create directory structure or flat structure
	// TODO: download license, too

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float statusBarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.y * 2.0f;

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - statusBarHeight));
	ImGui::SetNextWindowViewport(viewport->ID);
	{
		ui::ScopedStyle style;
		style.setWindowRounding(0.0f);
		style.setWindowBorderSize(0.0f);
		style.setWindowPadding(ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoTitleBar;

		static bool keepRunning = true;
		if (!ImGui::Begin(_app->fullAppname().c_str(), &keepRunning, windowFlags)) {
			ImGui::SetWindowCollapsed(ImGui::GetCurrentWindow(), false);
			ImGui::End();
			_app->minimize();
			return;
		}
		if (!keepRunning) {
			_app->requestQuit();
		}
	}

	ImGuiID dockIdMain = ImGui::GetID("DockSpace");
	const bool existingLayout = ImGui::DockBuilderGetNode(dockIdMain);
	ImGui::DockSpace(dockIdMain);

	updateAssetList(voxelFilesMap);
	updateAsset();
	updateAssetDetails();

	ImGui::End();

	_statusBar.update(TITLE_STATUSBAR, statusBarHeight);

	if (!existingLayout && viewport->WorkSize.x > 0.0f) {
		ImGui::DockBuilderAddNode(dockIdMain, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockIdMain, viewport->WorkSize);
		ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Left, 0.13f, nullptr, &dockIdMain);
		ImGuiID dockIdMainDown = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Down, 0.20f, nullptr, &dockIdMain);

		configureLeftTopWidgetDock(dockIdLeft);
		configureMainTopWidgetDock(dockIdMain);
		configureMainBottomWidgetDock(dockIdMainDown);

		ImGui::DockBuilderFinish(dockIdMain);
	}
}

bool MainWindow::init() {
	_texturePool.init();
	_thumbnailCtx.outputSize = glm::ivec2(1280);
	_thumbnailCtx.clearColor = glm::vec4(0.0f);
	return true;
}

void MainWindow::shutdown() {
	_filterEntries.clear();
	_texturePool.shutdown();
}

} // namespace voxbrowser
