/**
 * @file
 */

#include "TreePanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

namespace voxedit {

TreePanel::TreePanel() {
	switchTreeType(voxelgenerator::TreeType::Dome);
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

void TreePanel::update(const char *title) {
	static const struct {
		const char *name;
		voxelgenerator::TreeType type;
	} treeTypes[] = {
		{"Pine",				voxelgenerator::TreeType::Pine},
		{"Dome",				voxelgenerator::TreeType::Dome},
		{"Dome Hanging",		voxelgenerator::TreeType::DomeHangingLeaves},
		{"Cone",				voxelgenerator::TreeType::Cone},
		{"Fir",					voxelgenerator::TreeType::Fir},
		{"Ellipsis2",			voxelgenerator::TreeType::BranchesEllipsis},
		{"Ellipsis",			voxelgenerator::TreeType::Ellipsis},
		{"Cube",				voxelgenerator::TreeType::Cube},
		{"Cube Sides",			voxelgenerator::TreeType::CubeSideCubes},
		{"Palm",				voxelgenerator::TreeType::Palm},
		{"SpaceColonization",	voxelgenerator::TreeType::SpaceColonization}
	};
	static_assert(lengthof(treeTypes) == (int)voxelgenerator::TreeType::Max, "Missing support for tree types in the ui");

	if (ImGui::Begin(title)) {
		core_trace_scoped(TreePanel);
		if (ImGui::BeginCombo(ICON_FA_TREE " Type", treeTypes[core::enumVal(_treeGeneratorContext.cfg.type)].name, 0)) {
			for (int i = 0; i < lengthof(treeTypes); ++i) {
				if (ImGui::Selectable(treeTypes[i].name, i == core::enumVal(_treeGeneratorContext.cfg.type))) {
					switchTreeType((voxelgenerator::TreeType)i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::InputInt("Seed", (int*)&_treeGeneratorContext.cfg.seed);
		ImGui::InputInt("Trunk strength", &_treeGeneratorContext.cfg.trunkStrength);
		ImGui::InputInt("Trunk height", &_treeGeneratorContext.cfg.trunkHeight);
		ImGui::InputInt("Leaves width", &_treeGeneratorContext.cfg.leavesWidth);
		ImGui::InputInt("Leaves height", &_treeGeneratorContext.cfg.leavesHeight);
		ImGui::InputInt("Leaves depth", &_treeGeneratorContext.cfg.leavesDepth);
		switch (_treeGeneratorContext.cfg.type) {
		case voxelgenerator::TreeType::BranchesEllipsis:
			ImGui::InputInt("Branch length", &_treeGeneratorContext.branchellipsis.branchLength);
			ImGui::InputInt("Branch height", &_treeGeneratorContext.branchellipsis.branchHeight);
			break;
		case voxelgenerator::TreeType::Palm:
			ImGui::InputInt("Branch size", &_treeGeneratorContext.palm.branchSize);
			ImGui::InputInt("Trunk width", &_treeGeneratorContext.palm.trunkWidth);
			ImGui::InputInt("Trunk depth", &_treeGeneratorContext.palm.trunkDepth);
			ImGui::InputFloat("Branch reduction", &_treeGeneratorContext.palm.branchFactor);
			ImGui::InputFloat("Trunk reduction", &_treeGeneratorContext.palm.trunkFactor);
			ImGui::InputInt("Leaves", &_treeGeneratorContext.palm.branches);
			ImGui::InputInt("Bezier leaf", &_treeGeneratorContext.palm.branchControlOffset);
			ImGui::InputInt("Bezier trunk", &_treeGeneratorContext.palm.trunkControlOffset);
			ImGui::InputInt("Leaves h-offset", &_treeGeneratorContext.palm.randomLeavesHeightOffset);
			break;
		case voxelgenerator::TreeType::Fir:
			ImGui::InputInt("Branches", &_treeGeneratorContext.fir.branches);
			ImGui::InputFloat("W", &_treeGeneratorContext.fir.w);
			ImGui::InputInt("Amount", &_treeGeneratorContext.fir.amount);
			ImGui::InputFloat("Branch position factor", &_treeGeneratorContext.fir.branchPositionFactor);
			ImGui::InputInt("Branch strength", &_treeGeneratorContext.fir.branchStrength);
			ImGui::InputInt("Branch downward offset", &_treeGeneratorContext.fir.branchDownwardOffset);
			break;
		case voxelgenerator::TreeType::Pine:
			ImGui::InputInt("Start width", &_treeGeneratorContext.pine.startWidth);
			ImGui::InputInt("Start depth", &_treeGeneratorContext.pine.startDepth);
			ImGui::InputInt("Leaf height", &_treeGeneratorContext.pine.singleLeafHeight);
			ImGui::InputInt("Step delta", &_treeGeneratorContext.pine.singleStepDelta);
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			ImGui::InputInt("Branches", &_treeGeneratorContext.domehanging.branches);
			ImGui::InputInt("Leaves min length", &_treeGeneratorContext.domehanging.hangingLeavesLengthMin);
			ImGui::InputInt("Leaves max length", &_treeGeneratorContext.domehanging.hangingLeavesLengthMax);
			ImGui::InputInt("Leaves thickness", &_treeGeneratorContext.domehanging.hangingLeavesThickness);
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			ImGui::InputInt("Branch size", &_treeGeneratorContext.spacecolonization.branchSize);
			ImGui::InputFloat("Trunk reduction", &_treeGeneratorContext.spacecolonization.trunkFactor);
			break;
		default:
			break;
		}
		if (ImGui::Button(ICON_FA_CHECK " OK##treegenerate")) {
			_treeGeneratorContext.cfg.pos = sceneMgr().referencePosition();
			sceneMgr().createTree(_treeGeneratorContext);
		}
	}
	ImGui::End();
}

}
