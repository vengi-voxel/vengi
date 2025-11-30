/**
 * @file
 */

#pragma once

#include "IMGUIApp.h"
#include "color/ColorUtil.h"

namespace ui {

class ScopedStyle {
private:
	int _n = 0;
	int _font = 0;
	int _color = 0;
	const float _dpiScale;

public:
	ScopedStyle() : _dpiScale(ImGui::GetStyle().FontScaleDpi) {
	}
	~ScopedStyle() {
		ImGui::PopStyleVar(_n);
		ImGui::PopStyleColor(_color);
		resetFontSize();
	}
	inline void setColor(ImGuiCol idx, const ImVec4 &col) {
		ImGui::PushStyleColor(idx, col);
		++_color;
	}
	inline void resetColors(int n = 1) {
		ImGui::PopStyleColor(n);
		_color -= n;
	}
	inline void darker(ImGuiCol idx, float f = 1.0f) {
		setColor(idx, color::darker(ImGui::GetStyle().Colors[idx], f));
	}
	inline void brighter(ImGuiCol idx, float f = 1.0f) {
		setColor(idx, color::brighter(ImGui::GetStyle().Colors[idx], f));
	}
	inline ImVec4 highlight(const ImVec4 &c, float f = 1.0f) {
		if (c.x < 0.1f && c.y < 0.1f && c.z < 0.1f) {
			return color::brighter(c, f);
		}
		return color::darker(c, f);
	}
	/**
	 * Does either make the color brighter or darker depending on the current color
	 */
	inline void highlight(ImGuiCol idx, float f = 1.0f) {
		const ImVec4 &c = highlight(ImGui::GetStyle().Colors[idx], f);
		setColor(idx, c);
	}
	inline void pushFontSize(int size) {
		ImGui::PushFont(nullptr, size);
		++_font;
	}
	inline void resetFontSize() {
		if (_font > 0) {
			ImGui::PopFont();
			--_font;
		}
	}
	void setButtonColor(const ImVec4 &col) {
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		const ImVec4 hc = highlight(col, 1.5f);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hc);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hc);
		_color += 3;
	}
	inline void setAlpha(float alpha) {
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		++_n;
	}
	inline void setDisabledAlpha(float alpha) {
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, alpha);
		++_n;
	}
	inline void setWindowRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, val * _dpiScale);
		++_n;
	}
	inline void setWindowBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, val * _dpiScale);
		++_n;
	}
	inline void setChildRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, val * _dpiScale);
		++_n;
	}
	inline void setChildBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, val * _dpiScale);
		++_n;
	}
	inline void setPopupRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, val * _dpiScale);
		++_n;
	}
	inline void setPopupBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, val * _dpiScale);
		++_n;
	}
	inline void setFrameRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, val * _dpiScale);
		++_n;
	}
	inline void setFrameBorderSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, val * _dpiScale);
		++_n;
	}
	inline void setIndentSpacing(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, val * _dpiScale);
		++_n;
	}
	inline void setScrollbarSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, val * _dpiScale);
		++_n;
	}
	inline void setScrollbarRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, val * _dpiScale);
		++_n;
	}
	inline void setGrabMinSize(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, val * _dpiScale);
		++_n;
	}
	inline void setGrabRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, val * _dpiScale);
		++_n;
	}
	inline void setTabRounding(float val) {
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, val * _dpiScale);
		++_n;
	}
	inline void setWindowPadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setWindowMinSize(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setWindowTitleAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, val);
		++_n;
	}
	inline void setFramePadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setItemSpacing(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setItemInnerSpacing(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setCellPadding(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(val.x * _dpiScale, val.y * _dpiScale));
		++_n;
	}
	inline void setButtonTextAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, val);
		++_n;
	}
	inline void setSelectableTextAlign(const ImVec2 &val) {
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, val);
		++_n;
	}
};

class ScopedStyleCompact : public ScopedStyle {
public:
	ScopedStyleCompact() {
		ImGuiStyle &style = ImGui::GetStyle();
		setFramePadding(ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.60f)));
		setItemSpacing(ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.60f)));
	}
};

} // namespace ui
