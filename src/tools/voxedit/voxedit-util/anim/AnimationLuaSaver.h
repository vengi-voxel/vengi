/**
 * @file
 */

#pragma once

#include "core/io/FileStream.h"
#include "animation/CharacterSettings.h"

namespace voxedit {

bool saveCharacterLua(const animation::CharacterSettings& characterSettings, const io::FilePtr& file) {
	io::FileStream stream(file);
	stream.addStringFormat(false, "function init()\n"
		"  chr.setRace(\"%s\")\n"
		"  chr.setGender(\"%s\")\n"
		"  chr.setHead(\"%s\")\n"
		"  chr.setBelt(\"%s\")\n"
		"  chr.setChest(\"%s\")\n"
		"  chr.setPants(\"%s\")\n"
		"  chr.setHand(\"%s\")\n"
		"  chr.setFoot(\"%s\")\n"
		"  chr.setShoulder(\"%s\")\n",
		characterSettings.race.c_str(),
		characterSettings.gender.c_str(),
		characterSettings.path(animation::CharacterMeshType::Head),
		characterSettings.path(animation::CharacterMeshType::Belt),
		characterSettings.path(animation::CharacterMeshType::Chest),
		characterSettings.path(animation::CharacterMeshType::Pants),
		characterSettings.path(animation::CharacterMeshType::Hand),
		characterSettings.path(animation::CharacterMeshType::Foot),
		characterSettings.path(animation::CharacterMeshType::Shoulder));

	SkeletonAttribute dv;
	const SkeletonAttribute& sa = characterSettings.skeletonAttr;
	if (!glm::equal(sa.scaler, dv.scaler, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setScaler(%f)\n", sa.scaler);
	}
	if (!glm::equal(sa.headScale, dv.headScale, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setHeadScale(%f)\n", sa.headScale);
	}
	if (!glm::equal(sa.neckHeight, dv.neckHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setNeckHeight(%f)\n", sa.neckHeight);
	}
	if (!glm::equal(sa.neckForward, dv.neckForward, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setNeckForward(%f)\n", sa.neckForward);
	}
	if (!glm::equal(sa.neckRight, dv.neckRight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setNeckRight(%f)\n", sa.neckRight);
	}
	if (!glm::equal(sa.handForward, dv.handForward, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setHandForward(%f)\n", sa.handForward);
	}
	if (!glm::equal(sa.handRight, dv.handRight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setHandRight(%f)\n", sa.handRight);
	}
	if (!glm::equal(sa.shoulderForward, dv.shoulderForward, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setShoulderForward(%f)\n", sa.shoulderForward);
	}
	if (!glm::equal(sa.shoulderRight, dv.shoulderRight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setShoulderRight(%f)\n", sa.shoulderRight);
	}
	if (!glm::equal(sa.toolForward, dv.toolForward, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setToolForward(%f)\n", sa.toolForward);
	}
	if (!glm::equal(sa.toolRight, dv.toolRight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setToolRight(%f)\n", sa.toolRight);
	}
	if (!glm::equal(sa.toolScale, dv.toolScale, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setToolScale(%f)\n", sa.toolScale);
	}
	if (!glm::equal(sa.shoulderScale, dv.shoulderScale, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setShoulderScale(%f)\n", sa.shoulderScale);
	}
	if (!glm::equal(sa.headHeight, dv.headHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setHeadHeight(%f)\n", sa.headHeight);
	}
	if (!glm::equal(sa.footRight, dv.footRight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setFootRight(%f)\n", sa.footRight);
	}
	if (!glm::equal(sa.chestHeight, dv.chestHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setChestHeight(%f)\n", sa.chestHeight);
	}
	if (!glm::equal(sa.beltHeight, dv.beltHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setBeltHeight(%f)\n", sa.beltHeight);
	}
	if (!glm::equal(sa.pantsHeight, dv.pantsHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setPantsHeight(%f)\n", sa.pantsHeight);
	}
	if (!glm::equal(sa.invisibleLegHeight, dv.invisibleLegHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setInvisibleLegHeight(%f)\n", sa.invisibleLegHeight);
	}
	if (!glm::equal(sa.footHeight, dv.footHeight, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setFootHeight(%f)\n", sa.footHeight);
	}
	if (!glm::equal(sa.origin, dv.origin, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setOrigin(%f)\n", sa.origin);
	}
	if (!glm::equal(sa.hipOffset, dv.hipOffset, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setHipOffset(%f)\n", sa.hipOffset);
	}
	if (!glm::equal(sa.jumpTimeFactor, dv.jumpTimeFactor, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setJumpTimeFactor(%f)\n", sa.jumpTimeFactor);
	}
	if (!glm::equal(sa.runTimeFactor, dv.runTimeFactor, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setRunTimeFactor(%f)\n", sa.runTimeFactor);
	}
	if (!glm::equal(sa.idleTimeFactor, dv.idleTimeFactor, glm::epsilon<float>())) {
		stream.addStringFormat(false, "  chr.setIdleTimeFactor(%f)\n", sa.idleTimeFactor);
	}
	stream.addString("end\n", false);
	return true;
}


}
