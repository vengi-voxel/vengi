/**
 * @file
 */

#include "BrushPanelTexture.h"
#include "Style.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "command/CommandHandler.h"
#include "dearimgui/imgui_internal.h"
#include "image/Image.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/Panel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/brush/TextureBrush.h"
#include "voxelui/DragAndDropPayload.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

void BrushPanelTexture::update(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	Modifier &modifier = ctx.sceneMgr->modifier();
	TextureBrush &brush = modifier.textureBrush();
	core::String name = brush.image() ? core::string::extractFilenameWithExtension(brush.image()->name()) : _("None");
	ImGui::InputText(_("Texture"), &name, ImGuiInputTextFlags_ReadOnly);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ImagePayload)) {
			const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
			brush.setImage(image);
		}
		ImGui::EndDragDropTarget();
	}
	const bool itemClicked = ImGui::IsItemClicked();
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_FILE_INPUT) || itemClicked) {
		ctx.app->openDialog(
			[&](const core::String &filename, const io::FormatDescription *desc) {
				const image::ImagePtr &image = ctx.texturePool->loadImage(filename);
				brush.setImage(image);
			},
			{}, io::format::images());
	}

	const int nodeId = ctx.sceneMgr->sceneGraph().activeNode();
	ImGui::BeginDisabled(!ctx.sceneMgr->hasSelection(nodeId));
	ImGui::CommandIconButton(ICON_LC_SCAN, _("Use selection"), "texturebrushfromface", listener);
	ImGui::EndDisabled();

	bool projectOntoSurface = brush.projectOntoSurface();
	if (ImGui::Checkbox(_("Project onto surface"), &projectOntoSurface)) {
		brush.setProjectOntoSurface(projectOntoSurface);
	}

	glm::vec2 uv0 = brush.uv0();
	glm::vec2 uv1 = brush.uv1();
	if (brush.image()) {
		const video::TexturePtr &texture = ctx.texturePool->load(brush.image()->name());
		const glm::vec2 &imgSize = brush.image()->size();
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const glm::vec2 aspect(available.x / imgSize.x, available.y / imgSize.y);
		const float scale = core_min(aspect.x, aspect.y);
		const ImVec2 size = ImVec2(imgSize.x * scale, imgSize.y * scale);
		ImGui::InvisibleButton("#texturebrushimage", size);
		ImGui::AddImage(texture->handle(), uv0, uv1);
		ImGui::OpenPopupOnItemClick(POPUP_TITLE_UV_EDITOR, ImGuiPopupFlags_MouseButtonLeft);
	}
	if (ImGui::InputFloat2(_("UV min"), glm::value_ptr(uv0))) {
		brush.setUV0(uv0);
	}
	ImGui::TooltipTextUnformatted(_("Lower-left corner of the texture on the brush"));
	if (ImGui::InputFloat2(_("UV max"), glm::value_ptr(uv1))) {
		brush.setUV1(uv1);
	}
	ImGui::TooltipTextUnformatted(_("Upper-right corner of the texture on the brush"));
}

enum class UVEdge { UpperLeft, LowerRight, UpperRight, LowerLeft, Max };

static bool addUVHandle(UVEdge edge, const glm::ivec2 &mins, const glm::ivec2 &maxs, const glm::ivec2 &uiImageSize,
						uint32_t colorInt, float &u, float &v) {
	glm::ivec2 handlePos;
	switch (edge) {
	case UVEdge::UpperLeft:
		handlePos = mins;
		break;
	case UVEdge::LowerRight:
		handlePos = maxs;
		break;
	case UVEdge::UpperRight:
		handlePos = glm::ivec2(maxs.x, mins.y);
		break;
	case UVEdge::LowerLeft:
		handlePos = glm::ivec2(mins.x, maxs.y);
		break;
	default:
		return false;
	}
	const float size = ImGui::Size(1);
	const glm::ivec2 pos1(handlePos.x - size, handlePos.y - size);
	const glm::ivec2 pos2(handlePos.x + size, handlePos.y + size);
	bool retVal = false;
	const ImRect rect(pos1, pos2);
	const ImGuiID id = ImGui::GetCurrentWindow()->GetID((int)edge);
	if (!ImGui::ItemAdd(rect, id)) {
		return false;
	}

	bool hovered = false, held = false;
	/*bool clicked = */ ImGui::ButtonBehavior(rect, id, &hovered, &held);

	ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), colorInt, 0.0f,
										hovered ? 2.0f : 1.0f);
	if (held && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		const glm::ivec2 &pixelPos = image::Image::pixels({u, v}, uiImageSize.x, uiImageSize.y);
		const ImVec2 &mouseDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		const int px = glm::clamp((int)(pixelPos.x + mouseDelta.x), 0, uiImageSize.x - 1);
		const int py = glm::clamp((int)(pixelPos.y - mouseDelta.y), 0, uiImageSize.y - 1);
		const glm::vec2 uv = image::Image::uv(px, py, uiImageSize.x, uiImageSize.y);
		u = uv.x;
		v = uv.y;
		retVal = true;
	}
	return retVal;
}

void BrushPanelTexture::createPopups(BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
	const core::String title = ui::Panel::makeTitle(_("UV editor"), POPUP_TITLE_UV_EDITOR);
	bool showUVEditor = true;
	if (ImGui::BeginPopupModal(title.c_str(), &showUVEditor, ImGuiWindowFlags_AlwaysAutoResize)) {
		{
			ui::ScopedStyle style;
			style.pushFontSize(ctx.app->bigFontSize());
			ui::Toolbar toolbar("toolbar", &listener);
			toolbar.button(ICON_LC_FLIP_HORIZONTAL, "texturebrushmirroru");
			toolbar.button(ICON_LC_FLIP_VERTICAL, "texturebrushmirrorv");
			toolbar.button(ICON_LC_X, "texturebrushresetuv");
		}

		const glm::ivec2 cursor = ImGui::GetCursorScreenPos();
		Modifier &modifier = ctx.sceneMgr->modifier();
		TextureBrush &brush = modifier.textureBrush();
		const image::ImagePtr &image = brush.image();

		glm::vec2 uv0 = brush.uv0();
		glm::vec2 uv1 = brush.uv1();

		const video::TexturePtr &texture = ctx.texturePool->load(image->name());
		const float w = ImGui::Size(70);
		const float stretchFactor = w / image->width();
		const float h = image->height() * stretchFactor;
		const ImVec2 uiImageSize(w, h);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("#texturebrushimage", uiImageSize);
		ImGui::AddImage(texture->handle());
		const glm::ivec2 pixelMins =
			cursor + image::Image::pixels(uv0, w, h, image::TextureWrap::Repeat, image::TextureWrap::Repeat, true);
		const glm::ivec2 pixelMaxs =
			cursor + image::Image::pixels(uv1, w, h, image::TextureWrap::Repeat, image::TextureWrap::Repeat, true);
		const glm::vec4 color = ctx.app->color(style::StyleColor::ColorUVEditor) * 255.0f;
		const uint32_t colorInt = IM_COL32(color.r, color.g, color.b, color.a);

		bool dirty = false;
		ImGui::GetWindowDrawList()->AddRect(pixelMins, pixelMaxs, colorInt, 0.0f, 1.0f);
		if (addUVHandle(UVEdge::UpperLeft, pixelMins, pixelMaxs, uiImageSize, colorInt, uv0.x, uv0.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::LowerRight, pixelMins, pixelMaxs, uiImageSize, colorInt, uv1.x, uv1.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::UpperRight, pixelMins, pixelMaxs, uiImageSize, colorInt, uv1.x, uv0.y)) {
			dirty = true;
		}
		if (addUVHandle(UVEdge::LowerLeft, pixelMins, pixelMaxs, uiImageSize, colorInt, uv0.x, uv1.y)) {
			dirty = true;
		}
		if (dirty) {
			brush.setUV0(uv0);
			brush.setUV1(uv1);
		}

		ImGui::EndPopup();
	}
}

} // namespace voxedit
