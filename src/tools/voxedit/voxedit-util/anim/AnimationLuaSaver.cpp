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
	stream.addString("function init()", false);
	// TODO race and gender
	stream.addString("  chr.setBasePath(\"human\", \"male\")\n", false);
	stream.addString("  chr.setMeshTypes(", false);
	for (size_t i = 0; i < characterSettings.types().size(); ++i) {
		const std::string& type = characterSettings.types()[i];
		stream.addStringFormat(false, "\"%s\"", type.c_str());
		if (i != characterSettings.types().size() - 1) {
			stream.addString(", ", false);
		}
	}
	stream.addString("  )\n", false);
	for (const std::string& t : characterSettings.types()) {
		const int idx = characterSettings.getIdxForName(t.c_str());
		const std::string& path = characterSettings.path(idx, name);
		if (path.empty()) {
			continue;
		}
		stream.addStringFormat(false, "  chr.setPath(\"%s\", \"%s\")\n", t.c_str(), path.c_str());
	}
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
