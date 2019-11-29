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
	const std::string& headPath = characterSettings.path("head", name);
	const std::string& beltPath = characterSettings.path("belt", name);
	const std::string& chestPath = characterSettings.path("chest", name);
	const std::string& pantsPath = characterSettings.path("pants", name);
	const std::string& handPath = characterSettings.path("hand", name);
	const std::string& footPath = characterSettings.path("foot", name);
	const std::string& shoulderPath = characterSettings.path("shoulder", name);
	stream.addStringFormat(false, "function init()\n"
		"  chr.setRace(\"%s\")\n"
		"  chr.setGender(\"%s\")\n"
		"  chr.setPath(\"head\", \"%s\")\n"
		"  chr.setPath(\"belt\", \"%s\")\n"
		"  chr.setPath(\"chest\", \"%s\")\n"
		"  chr.setPath(\"pants\", \"%s\")\n"
		"  chr.setPath(\"hand\", \"%s\")\n"
		"  chr.setPath(\"foot\", \"%s\")\n"
		"  chr.setPath(\"shoulder\", \"%s\")\n",
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
