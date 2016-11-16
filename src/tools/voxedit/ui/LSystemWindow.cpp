/**
 * @file
 */

#include "LSystemWindow.h"
#include "voxel/generator/LSystemGenerator.h"
#include "editorscene/EditorScene.h"

namespace voxedit {

namespace {
voxel::lsystem::LSystemContext ctx;

class LSystemHighlighter : public tb::TBSyntaxHighlighter {
public:
	void OnBeforePaintFragment(const tb::TBPaintProps *props, tb::TBTextFragment *fragment) override {
		Log::info("Render %s", props->block->str.CStr());
	}

	void OnAfterPaintFragment(const tb::TBPaintProps *props, tb::TBTextFragment *fragment) override {
	}
};

}

LSystemWindow::LSystemWindow(ui::Window* window, EditorScene* scene) :
		Super(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-lsystem.tb.txt"));
}

void LSystemWindow::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);
	_axiom = getWidgetByType<tb::TBEditField>("axiom");
	_generations = getWidgetByType<tb::TBInlineSelect>("generations");

	if (_axiom == nullptr || _generations == nullptr) {
		Close();
		return;
	}

	LSystemHighlighter highlighter;
	_axiom->GetStyleEdit()->SetSyntaxHighlighter(&highlighter);
}

bool LSystemWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type != tb::EVENT_TYPE_CLICK) {
		return Super::OnEvent(ev);
	}
	if (ev.target->GetID() != TBIDC("ok")) {
		return Super::OnEvent(ev);
	}
	const tb::TBStr& axiomStr = _axiom->GetText();
	ctx.axiom = axiomStr.CStr();
	ctx.productionRules.emplace('A', ctx.axiom);
	ctx.voxels.emplace('A', voxel::createVoxel(voxel::VoxelType::Grass1));
	ctx.generations = _generations->GetValue();
	ctx.start = _scene->cursorPosition();
	Log::info("evaluate lsystem axiom %s with %i generations", ctx.axiom.c_str(), ctx.generations);
	_scene->lsystem(ctx);
	return true;
}

}
