/**
 * @file
 */

#include "Markdown.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "core/StringUtil.h"

namespace ImGui {

namespace _priv {

static void MarkdownLinkCallback(ImGui::MarkdownLinkCallbackData data) {
	if (data.isImage) {
		return;
	}
	const core::String link(data.link, data.linkLength);
	if (core::string::isUrl(link)) {
		command::executeCommands(core::String::format("url %s", link.c_str()));
		return;
	}
	Log::debug("Markdown link clicked: %s", link.c_str());
}

static ImGui::MarkdownImageData MarkdownImageCallback(ImGui::MarkdownLinkCallbackData data) {
	video::TexturePool *texturePool = (video::TexturePool *)data.userData;
	if (texturePool == nullptr) {
		ImGui::MarkdownImageData imageData;
		imageData.isValid = false;
		return imageData;
	}
	core::String imagePath(data.link, data.linkLength);
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
} // namespace _priv

void Markdown(const core::String &markdown, MarkdownLinkCallback *linkCallback, MarkdownImageCallback *imageCallback,
			  void *user) {
	if (markdown.empty()) {
		return;
	}
	ImGui::MarkdownConfig cfg;
	cfg.linkCallback = linkCallback;
	cfg.userData = user;
	cfg.tooltipCallback = nullptr;
	cfg.formatFlags = ImGuiMarkdownFormatFlags_GithubStyle;
	cfg.imageCallback = imageCallback;
	cfg.linkIcon = ICON_LC_LINK;
	const float fontSize = (float)imguiApp()->fontSize();
	auto &io = ImGui::GetIO();
	cfg.headingFormats[0] = {io.FontDefault, true, fontSize * 1.1f};
	cfg.headingFormats[1] = {io.FontDefault, true, fontSize};
	cfg.headingFormats[2] = {io.FontDefault, false, fontSize};
	ImGui::Markdown(markdown.c_str(), markdown.size(), cfg);
}

void Markdown(const core::String &markdown, video::TexturePool *texturePool) {
	Markdown(markdown, _priv::MarkdownLinkCallback, _priv::MarkdownImageCallback, texturePool);
}

} // namespace ImGui
