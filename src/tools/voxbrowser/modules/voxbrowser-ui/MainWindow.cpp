/**
 * @file
 */

#include "MainWindow.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "http/HttpCacheStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/PopupAbout.h"
#include "ui/ScopedStyle.h"
#include "video/Texture.h"
#include "voxelcollection/CollectionManager.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"

#define TITLE_STATUSBAR "##statusbar"
#define TITLE_ASSET "Asset##asset"
#define TITLE_ASSET_DETAILS "Details##asset-details"

namespace voxbrowser {

MainWindow::MainWindow(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool)
	: Super(app, "main"), _voxelCollection(app, texturePool), _statusBar(app), _menuBar(app), _texturePool(texturePool) {
}

MainWindow::~MainWindow() {
}

// https://github.com/ocornut/imgui/issues/6174
void MainWindow::image(const video::TexturePtr &texture) {
	const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();

	ImGui::InvisibleButton("##canvas", size,
						   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
							   ImGuiButtonFlags_MouseButtonMiddle);
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive(); // Held
	const ImGuiIO &io = ImGui::GetIO();

	const float zoomRate = 0.1f;
	const float zoomMouse = io.MouseWheel * zoomRate;
	const float zoomDelta = zoomMouse * _thumbnailProperties.scale.x;

	const ImVec2 oldScreenTopLeft = {cursorPos.x + _thumbnailProperties.translate.x,
									 cursorPos.y + _thumbnailProperties.translate.y};
	// on screen (center of what we get to see), when adjusting scale this doesn't change!
	const ImVec2 screenCenter = {cursorPos.x + size.x * 0.5f, cursorPos.y + size.y * 0.5f};
	// in image coordinate offset of the center
	const ImVec2 imageCenter = {screenCenter.x - oldScreenTopLeft.x, screenCenter.y - oldScreenTopLeft.y};

	const ImVec2 oldUVImageCenter = {imageCenter.x / (texture->width() * _thumbnailProperties.scale.x),
									 imageCenter.y / (texture->height() * _thumbnailProperties.scale.y)};

	if (hovered) {
		_thumbnailProperties.scale.x += zoomDelta;
		_thumbnailProperties.scale.y += zoomDelta;
	}

	_thumbnailProperties.scale.x = glm::clamp(_thumbnailProperties.scale.x, 0.01f, 100.0f);
	_thumbnailProperties.scale.y = glm::clamp(_thumbnailProperties.scale.y, 0.01f, 100.0f);

	// on screen new target center
	const ImVec2 newImageCenter = {(texture->width() * _thumbnailProperties.scale.x * oldUVImageCenter.x),
								   (texture->height() * _thumbnailProperties.scale.y * oldUVImageCenter.y)};

	// readjust to center
	_thumbnailProperties.translate.x -= newImageCenter.x - imageCenter.x;
	_thumbnailProperties.translate.y -= newImageCenter.y - imageCenter.y;

	// 0 out second parameter if a context menu is open
	if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0f)) {
		_thumbnailProperties.translate.x += ImGui::GetIO().MouseDelta.x;
		_thumbnailProperties.translate.y += ImGui::GetIO().MouseDelta.y;
	}

	const ImVec2 imageTopLeftPos(cursorPos.x + _thumbnailProperties.translate.x,
								 cursorPos.y + _thumbnailProperties.translate.y); // Lock scrolled origin

	// we need to control the rectangle we're going to draw and the uv coordinates
	const ImVec2 imageLowerRightPos = {imageTopLeftPos.x + (_thumbnailProperties.scale.x * texture->width()),
									   imageTopLeftPos.y + (_thumbnailProperties.scale.x * texture->height())};

	const ImVec2 lowerRightPos = ImVec2{cursorPos.x + size.x, cursorPos.y + size.y};
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(ImVec2{cursorPos.x + 2.0f, cursorPos.y + 2.0f},
						   ImVec2{lowerRightPos.x - 2.0f, lowerRightPos.y - 2.0f}, true);
	drawList->AddImage((ImTextureID)(intptr_t)texture->handle(), imageTopLeftPos, imageLowerRightPos);
	drawList->PopClipRect();
}

void MainWindow::updateAsset() {
	voxelcollection::VoxelFile &voxelFile = _voxelCollection.selected();

	if (ImGui::Begin(TITLE_ASSET, nullptr,
					 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_HorizontalScrollbar)) {
		if (voxelFile.name.empty()) {
			ui::ScopedStyle style;
			style.setFont(_app->bigFont());
			ImGui::TextCentered("Nothing selected");
		} else if (const video::TexturePtr &texture = _voxelCollection.thumbnailLookup(voxelFile)) {
			image(texture);
		} else {
			{
				ui::ScopedStyle style;
				style.setFont(_app->bigFont());
				ImGui::TextCentered("No thumbnail available", false);
			}

			if (voxelFile.downloaded && !_texturePool->has(voxelFile.name)) {
				createThumbnail(voxelFile);
			} else {
				if (ImGui::Button("Download")) {
					http::HttpCacheStream stream(_app->filesystem(), voxelFile.targetFile(), voxelFile.url);
					if (stream.valid()) {
						voxelFile.downloaded = true;
					} else {
						Log::warn("Failed to download %s", voxelFile.url.c_str());
					}
				}
			}
		}
	}
	ImGui::End();
}

void MainWindow::createThumbnail(const voxelcollection::VoxelFile &voxelFile) {
	scenegraph::SceneGraph sceneGraph;
	const core::String &fullPath = voxelFile.fullPath;
	io::FileDescription fileDesc;
	fileDesc.set(fullPath);
	voxelformat::LoadContext loadctx;
	const io::FilePtr &file = _app->filesystem()->open(fullPath, io::FileMode::SysRead);
	io::FileStream stream(file);
	if (!voxelformat::loadFormat(fileDesc, stream, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", file->name().c_str());
		return;
	}

	const core::String &targetImageFile = _app->filesystem()->writePath(voxelFile.targetFile() + ".png");
	if (!io::filesystem()->createDir(core::string::extractPath(targetImageFile))) {
		Log::warn("Failed to create directory for thumbnails at: %s", voxelFile.targetDir().c_str());
		return;
	}
	const image::ImagePtr &image = voxelrender::volumeThumbnail(sceneGraph, _thumbnailCtx);
	if (!image || image->isFailed()) {
		Log::error("Failed to create thumbnail for %s", voxelFile.name.c_str());
		return;
	}
	if (!image::writeImage(image, targetImageFile)) {
		Log::warn("Failed to save thumbnail for %s to %s", voxelFile.name.c_str(), targetImageFile.c_str());
	} else {
		Log::info("Created thumbnail for %s at %s", voxelFile.name.c_str(), targetImageFile.c_str());
	}
	image->setName(voxelFile.name);
	_texturePool->addImage(image);
}

void MainWindow::updateAssetDetails() {
	voxelcollection::VoxelFile &voxelFile = _voxelCollection.selected();
	if (ImGui::Begin(TITLE_ASSET_DETAILS)) {
		ImGui::Text("Name: %s", voxelFile.name.c_str());
		ImGui::Text("Source: %s", voxelFile.source.c_str());
		ImGui::Text("License: %s", voxelFile.license.c_str());
		if (!voxelFile.thumbnailUrl.empty()) {
			ImGui::URLItem("Thumbnail", voxelFile.thumbnailUrl.c_str());
		}
		ImGui::URLItem("URL", voxelFile.url.c_str());
		if (voxelFile.downloaded || io::filesystem()->open(voxelFile.fullPath, io::FileMode::SysRead)->exists()) {
			voxelFile.downloaded = true;
			if (ImGui::Button("Open")) {
				command::executeCommands("url \"file://" + voxelFile.fullPath + "\"");
			}
			if (!_texturePool->has(voxelFile.name)) {
				if (ImGui::Button("Create thumbnail")) {
					createThumbnail(voxelFile);
				}
			}
		} else {
			if (ImGui::Button("Download")) {
				http::HttpCacheStream stream(_app->filesystem(), voxelFile.targetFile(), voxelFile.url);
				if (stream.valid()) {
					voxelFile.downloaded = true;
				} else {
					Log::warn("Failed to download %s", voxelFile.url.c_str());
				}
			}
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
	ImGui::DockBuilderDockWindow(UI_CONSOLE_WINDOW_TITLE, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ASSET_DETAILS, dockId);
}

void MainWindow::registerPopups() {
	if (_menuBar._popupAbout) {
		ImGui::OpenPopup(POPUP_TITLE_ABOUT);
		_menuBar._popupAbout = false;
	}

	ui::popupAbout();
}

void MainWindow::update(voxelcollection::CollectionManager &collectionMgr) {
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

	if (_menuBar.update()) {
		ImGui::DockBuilderRemoveNode(dockIdMain);
	}

	const bool existingLayout = ImGui::DockBuilderGetNode(dockIdMain);
	ImGui::DockSpace(dockIdMain);

	ImGui::Begin(UI_CONSOLE_WINDOW_TITLE);
	ImGui::End();

	int n = 0;
	if (ImGui::Begin(TITLE_ASSET_LIST)) {
		n = _voxelCollection.update(collectionMgr);
	}
	ImGui::End();
	updateAsset();
	updateAssetDetails();

	registerPopups();

	ImGui::End();

#if 0
	if (ImGui::Begin("Texture Pool")) {
		const core::StringMap<video::TexturePtr> &cache = _texturePool.cache();
		for (const auto &entry : cache) {
			ImGui::Text("%s: width: %i height: %i", entry->first.c_str(), entry->second->width(),
						entry->second->height());
		}
	}
	ImGui::End();
#endif

	_statusBar.downloadProgress((float)collectionMgr.downloadProgress() / 100.0f);
	_statusBar.update(TITLE_STATUSBAR, statusBarHeight, n, collectionMgr.allEntries());

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
	_thumbnailCtx.outputSize = glm::ivec2(1280);
	_thumbnailCtx.clearColor = glm::vec4(0.0f);

	return true;
}

void MainWindow::shutdown() {
	_voxelCollection.shutdown();
}

} // namespace voxbrowser
