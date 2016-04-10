#pragma once

#include "ui/Window.h"
#include "core/Common.h"

using TREECTX = voxel::World::TreeContext;
static const ui::Window::Field TREEFIELDS[] = {
	{INT_FIELD("treetype", TREECTX, type)},
	{INT_FIELD("trunkheight", TREECTX, trunkHeight)},
	{INT_FIELD("trunkwidth", TREECTX, trunkWidth)},
	{INT_FIELD("leaveswidth", TREECTX, width)},
	{INT_FIELD("leavesheight", TREECTX, height)},
	{INT_FIELD("leavesdepth", TREECTX, depth)},
	{IVEC2_FIELD("treepos", TREECTX, pos)},
};

class TreeParametersWindow: public ui::Window {
private:
	ShapeTool* _tool;
	voxel::World::TreeContext _ctx;
public:
	TreeParametersWindow(ShapeTool* tool) :
			ui::Window(tool), _tool(tool) {
		core_assert(loadResourceFile("ui/window/treeparameters.tb.txt"));
		SetSettings(tb::WINDOW_SETTINGS_TITLEBAR);
		SetOpacity(0.5f);
		fillWidgets(TREEFIELDS, SDL_arraysize(TREEFIELDS), &_ctx);
		tb::TBSelectList *treeType = GetWidgetByIDAndType<tb::TBSelectList>("treetype");
		if (treeType != nullptr) {
			int max = static_cast<int>(voxel::World::TreeType::MAX);
			tb::TBGenericStringItemSource *itemSource = treeType->GetDefaultSource();
			for (int i = 0; i < max; ++i) {
				const tb::TBStr str(voxel::TreeTypeStr[i]);
				tb::TBGenericStringItem* item = new tb::TBGenericStringItem(str, tb::TBID(i));
				itemSource->AddItem(item);
			}
		}
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("ok")) || ev.special_key == tb::TB_KEY_ENTER) {
			fillFields(TREEFIELDS, SDL_arraysize(TREEFIELDS), &_ctx);
			_tool->placeTree(_ctx);
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};
