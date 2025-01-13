/**
 * @file
 */

#include "ThingNodeParser.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include <glm/gtc/type_ptr.hpp>

namespace voxelformat {

bool ThingNodeParser::parseChildren(core::Tokenizer &tok, NodeSpec &nodeSpec) const {
	while (tok.hasNext()) {
		NodeSpec child;
		if (parseNode(tok, child)) {
			nodeSpec.children.push_back(child);
		} else {
			Log::error("ThingFormat: Failed to parse child node");
			return false;
		}
		if (tok.isNext("{")) {
			tok.next();
			continue;
		}
		// no other children
		break;
	}
	return true;
}

static void skipBlock(core::Tokenizer &tok) {
	int depth = 1;
	while (tok.hasNext()) {
		core::String token = tok.next();
		if (token == "{") {
			depth++;
		} else if (token == "}") {
			depth--;
			if (depth == 0) {
				return;
			}
		}
	}
}

static bool parseAnimSpec(NodeSpec &nodeSpec, core::Tokenizer &tok) {
	if (!tok.hasNext()) {
		Log::error("ThingFormat: Expected token but got nothing");
		return false;
	}
	core::String token = tok.next();
	if (token != "{") {
		Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
		return false;
	}
	while (tok.hasNext()) {
		token = tok.next();
		if (token == "}") {
			return true;
		}
		if (token == "mode") {
			nodeSpec.animSpec.mode = core::string::toInt(tok.next());
		} else if (token == "startFrame") {
			nodeSpec.animSpec.startFrame = core::string::toInt(tok.next());
		} else if (token == "endFrame") {
			nodeSpec.animSpec.endFrame = core::string::toInt(tok.next());
		} else if (token == "fps") {
			nodeSpec.animSpec.fps = core::string::toInt(tok.next());
		} else if (token == "pause") {
			nodeSpec.animSpec.pause = core::string::toInt(tok.next());
		} else {
			Log::debug("ThingFormat: Ignoring token: '%s'", token.c_str());
		}
	}
	return true;
}

static bool parseRenderSpec(NodeSpec &nodeSpec, core::Tokenizer &tok) {
	if (!tok.hasNext()) {
		Log::error("ThingFormat: Expected token but got nothing");
		return false;
	}
	core::String token = tok.next();
	if (token != "{") {
		Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
		return false;
	}
	while (tok.hasNext()) {
		token = tok.next();
		if (token == "}") {
			return true;
		}
		if (token == "glowThresh") {
			nodeSpec.renderSpec.glowThresh = core::string::toFloat(tok.next());
		} else if (token == "glowIntensity") {
			nodeSpec.renderSpec.glowIntensity = core::string::toFloat(tok.next());
		} else {
			Log::debug("ThingFormat: Ignoring token: '%s'", token.c_str());
		}
	}
	return true;
}

static bool parseMediaCanvas(NodeSpec &nodeSpec, core::Tokenizer &tok) {
	if (!tok.hasNext()) {
		Log::error("ThingFormat: Expected token but got nothing");
		return false;
	}
	core::String token = tok.next();
	if (token != "{") {
		Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
		return false;
	}
	while (tok.hasNext()) {
		token = tok.next();
		if (token == "}") {
			return true;
		}
		if (token == "mediaStartTime") {
			nodeSpec.mediaCanvas.mediaStartTime = core::string::toInt(tok.next());
		} else if (token == "mediaVolume") {
			nodeSpec.mediaCanvas.mediaVolume = core::string::toInt(tok.next());
		} else if (token == "localPos") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.mediaCanvas.localPos), " ,\t");
		} else if (token == "localRot") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.mediaCanvas.localRot), " ,\t");
		} else if (token == "localScale") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.mediaCanvas.localScale), " ,\t");
		} else {
			Log::debug("ThingFormat: Ignoring token: '%s'", token.c_str());
		}
	}
	return true;
}

bool ThingNodeParser::parseNode(core::Tokenizer &tok, NodeSpec &nodeSpec) const {
	while (tok.hasNext()) {
		core::String token = tok.next();
		if (token.empty()) {
			continue;
		}
		if (token == "{") {
			skipBlock(tok);
			continue;
		}

		if (token == "}") {
			return true;
		} else if (token == "name") {
			nodeSpec.name = tok.next();
		} else if (token == "modelName") {
			nodeSpec.modelName = tok.next();
		} else if (token == "thingLibraryId") {
			nodeSpec.thingLibraryId = tok.next();
		} else if (token == "mediaName") {
			nodeSpec.mediaName = tok.next();
		} else if (token == "animSpec") {
			if (!parseAnimSpec(nodeSpec, tok)) {
				return false;
			}
		} else if (token == "renderSpec") {
			if (!parseRenderSpec(nodeSpec, tok)) {
				return false;
			}
		} else if (token == "mediaCanvas") {
			if (!parseMediaCanvas(nodeSpec, tok)) {
				return false;
			}
		} else if (token == "opacity") {
			nodeSpec.opacity = core::string::toFloat(tok.next());
		} else if (token == "children") {
			if (!tok.hasNext()) {
				Log::error("ThingFormat: Expected token but got nothing");
				return false;
			}
			token = tok.next();
			if (token != "{") {
				Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
				return false;
			}
			token = tok.next();
			if (token != "{") {
				Log::error("ThingFormat: Expected '{' but got: %s", token.c_str());
				return false;
			}
			if (!parseChildren(tok, nodeSpec)) {
				Log::error("Failed to parse children for node %s", nodeSpec.name.c_str());
			}
			token = tok.next();
			if (token != "}") {
				Log::error("ThingFormat: Expected '}' but got: %s", token.c_str());
				return false;
			}
		} else if (token == "color") {
			core::string::parseHex(tok.next().c_str(), nodeSpec.color.r, nodeSpec.color.g, nodeSpec.color.b,
								   nodeSpec.color.a);
		} else if (token == "localPos") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localPos), " ,\t");
		} else if (token == "localRot") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localRot), " ,\t");
		} else if (token == "localSize") {
			core::string::parseVec3(tok.next(), glm::value_ptr(nodeSpec.localSize), " ,\t");
		} else if (token == "scale") {
			nodeSpec.scale = core::string::toFloat(tok.next());
		} else {
			Log::debug("ThingFormat: Ignoring token: '%s'", token.c_str());
		}
	}
	return true;
}

bool ThingNodeParser::parseNode(const core::String &string, NodeSpec &nodeSpec) const {
	core::Tokenizer tok(string.c_str(), string.size(), ":");
	return parseNode(tok, nodeSpec);
}

} // namespace voxelformat
