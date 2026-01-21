/**
 * @file
 */
#pragma once

#include "core/String.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_markdown.h"
#include "video/TexturePool.h"

namespace ImGui {

IMGUI_API void Markdown(const core::String &markdown, video::TexturePool *texturePool);
IMGUI_API void Markdown(const core::String &markdown, MarkdownLinkCallback *linkCallback,
						MarkdownImageCallback *imageCallback, void *user);

} // namespace ImGui
