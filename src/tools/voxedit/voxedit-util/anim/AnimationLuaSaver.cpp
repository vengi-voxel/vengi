/**
 * @file
 */

#include "AnimationLuaSaver.h"
#include "core/io/FileStream.h"
#include "core/GLM.h"

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
	for (const SkeletonAttributeMeta* metaIter = SkeletonAttributeMetaArray; metaIter->name; ++metaIter) {
		const SkeletonAttributeMeta& meta = *metaIter;
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
