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
#include "ui/turbobadger/UIApp.h"
#include "core/JSON.h"
#include "voxel/MaterialColor.h"
#include "io/Filesystem.h"

namespace voxedit {

RuleItemSource productionRules;
voxel::lsystem::LSystemContext ctx;
Syntaxighlighter highlighter;

LSystemWindow::LSystemWindow(ui::turbobadger::Window* window, EditorScene* scene) :
		Super(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-lsystem.tb.txt"));
	_axiom = getWidgetByType<tb::TBEditField>("axiom");
	_generations = getWidgetByType<tb::TBInlineSelect>("generations");
	_productionRules = getWidgetByType<tb::TBSelectList>("productionrules");

	core_assert_msg(_axiom != nullptr, "TBEditField with name 'axiom' wasn't found");
	core_assert_msg(_generations != nullptr, "TBInlineSelect with name 'generations' wasn't found");
	core_assert_msg(_productionRules != nullptr, "TBSelectList with name 'productionrules' wasn't found");

	if (_axiom == nullptr || _generations == nullptr || _productionRules == nullptr) {
		Log::error("Not all needed widgets were found");
		Close();
		return;
	}

	_productionRules->SetSource(&productionRules);
	_productionRules->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_Y_AUTO);

	_axiom->GetStyleEdit()->SetSyntaxHighlighter(&highlighter);
	if (!ctx.axiom.empty()) {
		_axiom->SetText(ctx.axiom.c_str());
	}
}

LSystemWindow::~LSystemWindow() {
	if (_productionRules != nullptr) {
		_productionRules->SetSource(nullptr);
	}
}

bool LSystemWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("lsystem_ok")) {
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
			ctx.voxels.emplace('A', voxel::createRandomColorVoxel(voxel::VoxelType::Grass));
			ctx.generations = _generations->GetValue();
			ctx.start = _scene->referencePosition();
			Log::info("evaluate lsystem axiom %s with %i generations", ctx.axiom.c_str(), ctx.generations);
			_scene->lsystem(ctx);
			return true;
		} else if (ev.target->GetID() == TBIDC("lsystem_cancel")) {
			tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
			m_close_button.InvokeEvent(click_ev);
			return true;
		} else if (ev.target->GetID() == TBIDC("lsystem_add_rule")) {
			tb::TBWidget* wStr = getWidget("lsystem_add_rule_string");
			tb::TBWidget* wChar = getWidget("lsystem_add_rule_character");
			if (wStr != nullptr && wChar != nullptr) {
				const tb::TBStr& str = wStr->GetText();
				const tb::TBStr& character = wChar->GetText();
				productionRules.AddItem(new RuleItem(str.CStr(), character[0]));
			}
			return true;
		} else if (ev.target->GetID() == TBIDC("lsystem_load")) {
			load(((ui::turbobadger::UIApp*)core::App::getInstance())->openDialog("txt"));
			return true;
		} else if (ev.target->GetID() == TBIDC("lsystem_save")) {
			save(((ui::turbobadger::UIApp*)core::App::getInstance())->saveDialog("txt"));
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			tb::TBWidgetEvent click_ev(tb::EVENT_TYPE_CLICK);
			m_close_button.InvokeEvent(click_ev);
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (ev.target->GetID() == TBIDC("lsystem_add_rule_character")) {
			const tb::TBStr& str = ev.target->GetText();
			if (str.Length() > 1) {
				const char buf[] = {str[0], '\0'};
				ev.target->SetText(buf);
			}
		}
	}
	return Super::OnEvent(ev);
}

void LSystemWindow::save(const std::string& file) {
	if (file.empty()) {
		return;
	}

	const tb::TBStr& axiom = _axiom->GetText();
	const int generations = _generations->GetValue();
	const int n = productionRules.GetNumItems();
	std::vector<core::json> rules;
	for (int i = 0; i < n; ++i) {
		RuleItem* item = productionRules.GetItem(i);
		const char character = item->character();
		const tb::TBStr& productionRule = item->str;
		const char buf[] = {character, '\0'};
		const core::json jsonRule = {
			{"character", buf},
			{"rule", productionRule.CStr()}
		};
		rules.emplace_back(jsonRule);
	}
	std::vector<core::json> voxels;
	// TODO: save voxels

	core::json j = {
		{"axiom", axiom.CStr()},
		{"generations", generations},
		{"voxels", voxels},
		{"rules", rules},
	};
	const std::string& jsonStr = j.dump(4);
	if (!core::App::getInstance()->filesystem()->syswrite(file, jsonStr)) {
		Log::error("Failed to write file %s", file.c_str());
	} else {
		Log::info("Saved file %s", file.c_str());
	}
	Log::info("%s", jsonStr.c_str());
}

void LSystemWindow::load(const std::string& file) {
	if (file.empty()) {
		return;
	}
	const std::string& jsonStr = core::App::getInstance()->filesystem()->load(file);
	if (jsonStr.empty()) {
		return;
	}
	try {
		core::json j = core::json::parse(jsonStr);
		const std::string axiom = j["axiom"];
		_axiom->SetText(axiom.c_str());
		const int generations = j["generations"];
		_generations->SetValue(generations);
		productionRules.DeleteAllItems();
		const auto& rules = j["rules"];
		for (auto& jsonRule: rules) {
			const std::string& chr = jsonRule["character"];
			const std::string& rule = jsonRule["rule"];
			productionRules.AddItem(new RuleItem(rule.c_str(), chr[0]));
		}
		const auto& voxels = j["voxels"];
		for (auto& jsonVoxel: voxels) {
			const std::string& chr = jsonVoxel["character"];
			const int rawValue = jsonVoxel["type"];
			const int max = voxel::getMaterialColors().size();
			if (rawValue < 0 || rawValue >= max) {
				Log::warn("Skip %s with type %i", chr.c_str(), rawValue);
				continue;
			}
			const voxel::VoxelType type = (voxel::VoxelType)rawValue;
			// TODO: add voxels
		}
	} catch (...) {
		Log::error("Failed to parse %s", file.c_str());
	}
}

}
