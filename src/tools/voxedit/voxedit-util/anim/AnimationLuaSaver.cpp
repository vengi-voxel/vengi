/**
 * @file
 */

#include "AnimationLuaSaver.h"
#include "animation/SkeletonAttribute.h"
#include "core/io/FileStream.h"
#include "core/GLM.h"

namespace voxedit {

bool saveCharacterLua(const animation::AnimationSettings& settings, const animation::CharacterSkeletonAttribute& sa, const char *name, const io::FilePtr& file) {
	if (!file || !file->exists()) {
		return false;
	}
	io::FileStream stream(file);
	stream.addString("function init()", false);
	// TODO race and gender
	stream.addString("  settings.setBasePath(\"human\", \"male\")\n", false);
	stream.addString("  settings.setMeshTypes(", false);
	for (size_t i = 0; i < settings.types().size(); ++i) {
		const std::string& type = settings.types()[i];
		stream.addStringFormat(false, "\"%s\"", type.c_str());
		if (i != settings.types().size() - 1) {
			stream.addString(", ", false);
		}
	}
	stream.addString("  )\n", false);
	for (const std::string& t : settings.types()) {
		const int idx = settings.getIdxForName(t.c_str());
		const std::string& path = settings.path(idx, name);
		if (path.empty()) {
			continue;
		}
		stream.addStringFormat(false, "  settings.setPath(\"%s\", \"%s\")\n", t.c_str(), path.c_str());
	}
	animation::CharacterSkeletonAttribute dv;
	for (const animation::SkeletonAttributeMeta* metaIter = animation::ChrSkeletonAttributeMetaArray; metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		const float *saVal = (const float*)(((const char*)&sa) + meta.offset);
		const float *dvVal = (const float*)(((const char*)&dv) + meta.offset);
		if (glm::abs(*saVal - *dvVal) > glm::epsilon<float>()) {
			stream.addStringFormat(false, "  %s = %f", meta.name, *saVal);
		}
		if ((metaIter + 1)->name) {
			stream.addString(",\n", false);
		} else {
			stream.addString("\n", false);
		}
	}
	stream.addString("end\n", false);
	return true;
}

}
