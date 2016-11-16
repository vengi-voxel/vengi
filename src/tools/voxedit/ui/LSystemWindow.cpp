/**
 * @file
 */

#include "LSystemWindow.h"
#include "voxel/generator/LSystemGenerator.h"
#include "editorscene/EditorScene.h"
#include "lsystem/RuleItem.h"
#include "lsystem/RuleItemSource.h"
#include "lsystem/RuleItemWidget.h"
#include "lsystem/SyntaxHighlighter.h"

namespace voxedit {

RuleItemSource productionRules;
voxel::lsystem::LSystemContext ctx;
Syntaxighlighter highlighter;

LSystemWindow::LSystemWindow(ui::Window* window, EditorScene* scene) :
		Super(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-lsystem.tb.txt"));
	_axiom = getWidgetByType<tb::TBEditField>("axiom");
	_generations = getWidgetByType<tb::TBInlineSelect>("generations");
	_productionRules = getWidgetByType<tb::TBSelectList>("productionrules");

	core_assert_msg(_axiom != nullptr, "TBEditField with name 'axiom' wasn't found");
	core_assert_msg(_generations != nullptr, "TBInlineSelect with name 'generations' wasn't found");
	core_assert_msg(_productionRules != nullptr, "TBSelectList with name 'productionrules' wasn't found");

	if (_axiom == nullptr || _generations == nullptr || _productionRules == nullptr) {
		Close();
		return;
	}

	_productionRules->SetSource(&productionRules);
	_productionRules->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_Y_AUTO);

	_axiom->GetStyleEdit()->SetSyntaxHighlighter(&highlighter);
}

bool LSystemWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			const tb::TBStr& axiomStr = _axiom->GetText();
			ctx.axiom = axiomStr.CStr();
			const int n = productionRules.GetNumItems();
			for (int i = 0; i < n; ++i) {
				RuleItem* item = productionRules.GetItem(i);
				const char c = item->character();
				const char* str = item->str.CStr();
				ctx.productionRules.emplace(c, str);
			}
			// TODO: voxels via ui
			ctx.voxels.emplace('A', voxel::createVoxel(voxel::VoxelType::Grass1));
			ctx.generations = _generations->GetValue();
			ctx.start = _scene->cursorPosition();
			Log::info("evaluate lsystem axiom %s with %i generations", ctx.axiom.c_str(), ctx.generations);
			_scene->lsystem(ctx);
			return true;
		} else if (ev.target->GetID() == TBIDC("add_rule")) {
			productionRules.AddItem(new RuleItem("Ax", 'A'));
		}
	}
	return Super::OnEvent(ev);
}

}
