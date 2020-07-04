/**
 * @file
 */

#include "AnimationLuaSaver.h"
#include "animation/SkeletonAttribute.h"
#include "core/io/FileStream.h"
#include "core/GLM.h"
#include <glm/gtc/constants.hpp>
#include <glm/common.hpp>
#include "core/Assert.h"
#include "core/Log.h"

namespace voxedit {

#define wrapBool(expression) \
	if (!(expression)) { \
		Log::error("Error: writing animation entity lua at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

bool saveAnimationEntityLua(const animation::AnimationSettings& settings, const animation::SkeletonAttribute& sa, const char *name, const io::FilePtr& file) {
	if (!file || !file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	wrapBool(stream.addString("require 'chr.bones'\n", false))
	wrapBool(stream.addString("require 'chr.shared'\n\n", false))
	wrapBool(stream.addString("function init()\n", false))
	// TODO race and gender
	wrapBool(stream.addString("  settings.setBasePath(\"human\", \"male\")\n", false))
	wrapBool(stream.addString("  settings.setMeshTypes(", false))
	for (size_t i = 0; i < settings.types().size(); ++i) {
		const core::String& type = settings.types()[i];
		wrapBool(stream.addStringFormat(false, "\"%s\"", type.c_str()))
		if (i != settings.types().size() - 1) {
			wrapBool(stream.addString(", ", false))
		}
	}
	wrapBool(stream.addString("  )\n", false))
	for (const core::String& t : settings.types()) {
		const int meshTypeIdx = settings.getMeshTypeIdxForName(t.c_str());
		const core::String& path = settings.path(meshTypeIdx, name);
		if (path.empty()) {
			continue;
		}
		wrapBool(stream.addStringFormat(false, "  settings.setPath(\"%s\", \"%s\")\n", t.c_str(), path.c_str()))
	}

	wrapBool(stream.addString("  local attributes = defaultSkeletonAttributes()\n", false))
	const animation::CharacterSkeletonAttribute dv {};
	for (const animation::SkeletonAttributeMeta* metaIter = sa.metaArray(); metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		const float *saVal = (const float*)(((const uint8_t*)&sa) + meta.offset);
		const float *dvVal = (const float*)(((const uint8_t*)&dv) + meta.offset);
		if (glm::abs(*saVal - *dvVal) > glm::epsilon<float>()) {
			wrapBool(stream.addStringFormat(false, "  attributes[\"%s\"] = %f\n", meta.name, *saVal))
		}
	}
	wrapBool(stream.addString("  return attributes\n", false))
	wrapBool(stream.addString("end\n", false))
	return true;
}

#undef wrapBool

}
