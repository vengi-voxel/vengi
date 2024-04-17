/**
 * @file
 */

#include "TreePanel.h"
#include "IconsLucide.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool TreePanel::init() {
	switchTreeType(voxelgenerator::TreeType::Dome);
	// TODO: load the tree settings
	return true;
}

void TreePanel::switchTreeType(voxelgenerator::TreeType treeType) {
	switch (treeType) {
		case voxelgenerator::TreeType::Dome:
			_treeGeneratorContext.dome = voxelgenerator::TreeDome();
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			_treeGeneratorContext.domehanging = voxelgenerator::TreeDomeHanging();
			break;
		case voxelgenerator::TreeType::Cone:
			_treeGeneratorContext.cone = voxelgenerator::TreeCone();
			break;
		case voxelgenerator::TreeType::Ellipsis:
			_treeGeneratorContext.ellipsis = voxelgenerator::TreeEllipsis();
			break;
		case voxelgenerator::TreeType::BranchesEllipsis:
			_treeGeneratorContext.branchellipsis = voxelgenerator::TreeBranchEllipsis();
			break;
		case voxelgenerator::TreeType::Cube:
		case voxelgenerator::TreeType::CubeSideCubes:
			_treeGeneratorContext.cube = voxelgenerator::TreeCube();
			break;
		case voxelgenerator::TreeType::Pine:
			_treeGeneratorContext.pine = voxelgenerator::TreePine();
			break;
		case voxelgenerator::TreeType::Fir:
			_treeGeneratorContext.fir = voxelgenerator::TreeFir();
			break;
		case voxelgenerator::TreeType::Palm:
			_treeGeneratorContext.palm = voxelgenerator::TreePalm();
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			_treeGeneratorContext.spacecolonization = voxelgenerator::TreeSpaceColonization();
			break;
		case voxelgenerator::TreeType::Max:
		default:
			break;
	}
	_treeGeneratorContext.cfg.type = treeType;
}

const char *TreePanel::treeTypeName(int i) const {
	// I18N: by having this static, this requires a restart to change the language
	static const char *treeTypes[] = {_("Pine"),
									  _("Dome"),
									  _("Dome Hanging Leaves"),
									  _("Cone"),
									  _("Fir"),
									  _("Branches Ellipsis"),
									  _("Ellipsis"),
									  _("Cube"),
									  _("Cube Sides"),
									  _("Palm"),
									  _("SpaceColonization")};
	static_assert(lengthof(treeTypes) == (int)voxelgenerator::TreeType::Max,
				  "Missing support for tree types in the ui");
	return treeTypes[i];
}

void TreePanel::update(const char *title) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(TreePanel);
		if (ImGui::BeginIconCombo(ICON_LC_TREES, _("Type"), treeTypeName(core::enumVal(_treeGeneratorContext.cfg.type)), 0)) {
			for (int i = 0; i < (int)voxelgenerator::TreeType::Max; ++i) {
				if (ImGui::Selectable(treeTypeName(i), i == core::enumVal(_treeGeneratorContext.cfg.type))) {
					switchTreeType((voxelgenerator::TreeType)i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::InputInt(_("Seed"), (int *)&_treeGeneratorContext.cfg.seed);
		ImGui::InputInt(_("Trunk strength"), &_treeGeneratorContext.cfg.trunkStrength);
		ImGui::InputInt(_("Trunk height"), &_treeGeneratorContext.cfg.trunkHeight);
		ImGui::InputInt(_("Leaves width"), &_treeGeneratorContext.cfg.leavesWidth);
		ImGui::InputInt(_("Leaves height"), &_treeGeneratorContext.cfg.leavesHeight);
		ImGui::InputInt(_("Leaves depth"), &_treeGeneratorContext.cfg.leavesDepth);
		switch (_treeGeneratorContext.cfg.type) {
		case voxelgenerator::TreeType::BranchesEllipsis:
			ImGui::InputInt(_("Branch length"), &_treeGeneratorContext.branchellipsis.branchLength);
			ImGui::InputInt(_("Branch height"), &_treeGeneratorContext.branchellipsis.branchHeight);
			break;
		case voxelgenerator::TreeType::Palm:
			ImGui::InputInt(_("Branch size"), &_treeGeneratorContext.palm.branchSize);
			ImGui::InputInt(_("Trunk width"), &_treeGeneratorContext.palm.trunkWidth);
			ImGui::InputInt(_("Trunk depth"), &_treeGeneratorContext.palm.trunkDepth);
			ImGui::InputFloat(_("Branch reduction"), &_treeGeneratorContext.palm.branchFactor);
			ImGui::InputFloat(_("Trunk reduction"), &_treeGeneratorContext.palm.trunkFactor);
			ImGui::InputInt(_("Leaves"), &_treeGeneratorContext.palm.branches);
			ImGui::InputInt(_("Bezier leaf"), &_treeGeneratorContext.palm.branchControlOffset);
			ImGui::InputInt(_("Bezier trunk"), &_treeGeneratorContext.palm.trunkControlOffset);
			ImGui::InputInt(_("Leaves h-offset"), &_treeGeneratorContext.palm.randomLeavesHeightOffset);
			break;
		case voxelgenerator::TreeType::Fir:
			ImGui::InputInt(_("Branches"), &_treeGeneratorContext.fir.branches);
			ImGui::InputFloat(_("W"), &_treeGeneratorContext.fir.w);
			ImGui::InputInt(_("Amount"), &_treeGeneratorContext.fir.amount);
			ImGui::InputFloat(_("Branch position factor"), &_treeGeneratorContext.fir.branchPositionFactor);
			ImGui::InputInt(_("Branch strength"), &_treeGeneratorContext.fir.branchStrength);
			ImGui::InputInt(_("Branch downward offset"), &_treeGeneratorContext.fir.branchDownwardOffset);
			break;
		case voxelgenerator::TreeType::Pine:
			ImGui::InputInt(_("Start width"), &_treeGeneratorContext.pine.startWidth);
			ImGui::InputInt(_("Start depth"), &_treeGeneratorContext.pine.startDepth);
			ImGui::InputInt(_("Leaf height"), &_treeGeneratorContext.pine.singleLeafHeight);
			ImGui::InputInt(_("Step delta"), &_treeGeneratorContext.pine.singleStepDelta);
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			ImGui::InputInt(_("Branches"), &_treeGeneratorContext.domehanging.branches);
			ImGui::InputInt(_("Leaves min length"), &_treeGeneratorContext.domehanging.hangingLeavesLengthMin);
			ImGui::InputInt(_("Leaves max length"), &_treeGeneratorContext.domehanging.hangingLeavesLengthMax);
			ImGui::InputInt(_("Leaves thickness"), &_treeGeneratorContext.domehanging.hangingLeavesThickness);
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			ImGui::InputInt(_("Branch size"), &_treeGeneratorContext.spacecolonization.branchSize);
			ImGui::InputFloat(_("Trunk reduction"), &_treeGeneratorContext.spacecolonization.trunkFactor);
			break;
		default:
			break;
		}
		if (ImGui::IconButton(ICON_LC_CHECK, _("OK"))) {
			_treeGeneratorContext.cfg.pos = _sceneMgr->referencePosition();
			_sceneMgr->createTree(_treeGeneratorContext);
		}
	}
	ImGui::End();
}

void TreePanel::shutdown() {
	// TODO: persist the tree settings
}

} // namespace voxedit
