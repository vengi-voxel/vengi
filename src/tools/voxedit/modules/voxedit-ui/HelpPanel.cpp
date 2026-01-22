/**
 * @file
 */

#include "HelpPanel.h"
#include "IconsLucide.h"
#include "core/StringUtil.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "ui/Markdown.h"
#include "voxedit-ui/MainWindow.h"

namespace voxedit {

void HelpPanel::init() {
	State state;
	state._basePath = core::string::path("docs/voxedit");
	state._filename = "Index.md";
	_history.clear();
	_history.push_back(state);
	_historyPosition = 0;
	loadCurrentState();
}

void HelpPanel::setMarkdownState(const State &state) {
	// Remove any forward history when navigating to a new page
	if (_historyPosition < (int)_history.size() - 1) {
		size_t removeCount = _history.size() - _historyPosition - 1;
		_history.erase(_historyPosition + 1, removeCount);
	}
	_history.push_back(state);
	_historyPosition = (int)_history.size() - 1;
	loadCurrentState();
}

void HelpPanel::loadCurrentState() {
	const io::FilesystemPtr &fs = _app->filesystem();
	const core::String &path = core::string::path(c()._basePath, c()._filename);
	Log::debug("Loading markdown file '%s'", path.c_str());
	const io::FilePtr &markdownFile = fs->open(path);
	if (!markdownFile || !markdownFile->validHandle()) {
		Log::warn("Markdown file '%s' not found", path.c_str());
		return;
	}
	const core::String &markdown = markdownFile->load();
	if (markdown.empty()) {
		Log::warn("Failed to load markdown file '%s'", path.c_str());
		return;
	}
	setMarkdown(markdown);
}

void HelpPanel::setMarkdownFile(const core::String &file) {
	const io::FilesystemPtr &fs = _app->filesystem();
	const core::String &path = core::string::path(c()._basePath, file);
	const io::FilePtr &markdownFile = fs->open(path);
	if (!markdownFile) {
		Log::warn("Markdown file '%s' not found", path.c_str());
		return;
	}
	State state;
	state._basePath = markdownFile->dir();
	state._filename = core::string::extractFilenameWithExtension(markdownFile->name());
	markdownFile->close();
	setMarkdownState(state);
}

const video::TexturePoolPtr &HelpPanel::texturePool() const {
	return _mainWindow->texturePool();
}

static void linkCallback(ImGui::MarkdownLinkCallbackData data) {
	if (data.isImage) {
		return;
	}
	HelpPanel *panel = (HelpPanel *)data.userData;
	const core::String link(data.link, data.linkLength);
	if (core::string::isUrl(link)) {
		command::executeCommands(core::String::format("url %s", link.c_str()));
		return;
	}
	panel->setMarkdownFile(link);
}

static ImGui::MarkdownImageData imageCallback(ImGui::MarkdownLinkCallbackData data) {
	HelpPanel *panel = (HelpPanel *)data.userData;
	const HelpPanel::State &c = panel->c();
	video::TexturePool *texturePool = panel->texturePool().get();
	core::String imagePath(data.link, data.linkLength);
	imagePath = core::string::path(c._basePath, imagePath);
	if (texturePool == nullptr) {
		ImGui::MarkdownImageData imageData;
		imageData.isValid = false;
		return imageData;
	}
	video::TexturePtr texture = texturePool->load(imagePath);
	ImGui::MarkdownImageData imageData;
	imageData.isValid = texture->isLoaded();
	if (!imageData.isValid) {
		return imageData;
	}
	imageData.useLinkCallback = false;
	imageData.user_texture_id = (ImTextureID)(intptr_t)texture->handle();
	imageData.size = ImVec2(texture->width(), texture->height()); // TODO: dpi

	const ImVec2 contentSize = ImGui::GetContentRegionAvail();
	if (imageData.size.x > contentSize.x) {
		const float ratio = imageData.size.y / imageData.size.x;
		imageData.size.x = contentSize.x;
		imageData.size.y = contentSize.x * ratio;
	}

	return imageData;
}

void HelpPanel::goBack() {
	if (canGoBack()) {
		--_historyPosition;
		loadCurrentState();
	}
}

void HelpPanel::goForward() {
	if (canGoForward()) {
		++_historyPosition;
		loadCurrentState();
	}
}

bool HelpPanel::canGoBack() const {
	return _historyPosition > 0;
}

bool HelpPanel::canGoForward() const {
	return _historyPosition < (int)_history.size() - 1;
}

void HelpPanel::navigation() {
	bool backDisabled = !canGoBack();
	if (backDisabled) {
		ImGui::BeginDisabled();
	}
	if (ImGui::IconButton(ICON_LC_ARROW_LEFT, _("Back"))) {
		goBack();
	}
	if (backDisabled) {
		ImGui::EndDisabled();
	}

	ImGui::SameLine();

	bool forwardDisabled = !canGoForward();
	if (forwardDisabled) {
		ImGui::BeginDisabled();
	}
	if (ImGui::IconButton(ICON_LC_ARROW_RIGHT, _("Forward"))) {
		goForward();
	}
	if (forwardDisabled) {
		ImGui::EndDisabled();
	}

	if (!backDisabled || !forwardDisabled) {
		ImGui::SameLine();

		if (ImGui::IconButton(ICON_LC_HOUSE, _("Home"))) {
			init();
		}
	}

	ImGui::Separator();
}

void HelpPanel::update(const char *id) {
	// Apply any pending markdown updates after previous ImGui::Markdown processing is complete - this is needed because
	// the markdown link processing might change the content.
	if (_hasPendingUpdate) {
		_markdown = _pendingMarkdown;
		_pendingMarkdown.clear();
		_hasPendingUpdate = false;
	}

	if (_markdown.empty()) {
		return;
	}
	core_trace_scoped(HelpPanel);
	const core::String title = makeTitle(ICON_LC_LAMP, _("Help"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		navigation();
		ImGui::Markdown(_markdown, linkCallback, imageCallback, this);
	}
	ImGui::End();
}

} // namespace voxedit
