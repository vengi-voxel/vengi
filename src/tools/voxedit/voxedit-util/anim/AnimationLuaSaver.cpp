/**
 * @file
 */

#include "AnimationLuaSaver.h"
#include "animation/SkeletonAttribute.h"
#include "core/io/FileStream.h"
#include "core/GLM.h"

namespace voxedit {

bool saveCharacterLua(const animation::CharacterSettings& characterSettings, const char *name, const io::FilePtr& file) {
	if (!file || !file->exists()) {
		return false;
	}
	io::FileStream stream(file);
	const std::string& headPath = characterSettings.path(animation::CharacterMeshType::Head, name);
	const std::string& beltPath = characterSettings.path(animation::CharacterMeshType::Belt, name);
	const std::string& chestPath = characterSettings.path(animation::CharacterMeshType::Chest, name);
	const std::string& pantsPath = characterSettings.path(animation::CharacterMeshType::Pants, name);
	const std::string& handPath = characterSettings.path(animation::CharacterMeshType::Hand, name);
	const std::string& footPath = characterSettings.path(animation::CharacterMeshType::Foot, name);
	const std::string& shoulderPath = characterSettings.path(animation::CharacterMeshType::Shoulder, name);
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
		headPath.c_str(),
		beltPath.c_str(),
		chestPath.c_str(),
		pantsPath.c_str(),
		handPath.c_str(),
		footPath.c_str(),
		shoulderPath.c_str());

	animation::CharacterSkeletonAttribute dv;
	const animation::CharacterSkeletonAttribute& sa = characterSettings.skeletonAttr;
	for (const animation::SkeletonAttributeMeta* metaIter = animation::ChrSkeletonAttributeMetaArray; metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		const float *saVal = (const float*)(((const char*)&sa) + meta.offset);
		const float *dvVal = (const float*)(((const char*)&dv) + meta.offset);
		if (glm::abs(*saVal - *dvVal) > glm::epsilon<float>()) {
			stream.addStringFormat(false, "  chr.set%s(%f)\n", meta.name, *saVal);
		}
	}
	stream.addString("end\n", false);
	return true;
}

}
