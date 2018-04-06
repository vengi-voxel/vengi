/**
 * @file
 */

#include "Parser.h"
#include "core/Array.h"
#include "core/Common.h"
#include "core/String.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "video/Shader.h"
#include "Generator.h"
#include "Parser.h"
#include "Util.h"

namespace shadertool {

static const char* PrimitiveTypeStr[] {
	nullptr,
	"points",
	"lines",
	"lines_adjacency",
	"triangles",
	"triangles_adjacency",
	"line_strip",
	"triangle_strip"
};
static_assert(lengthof(PrimitiveTypeStr) == std::enum_value(PrimitiveType::Max), "PrimitiveTypeStr doesn't match enum");

static PrimitiveType layoutPrimitiveType(const std::string& token) {
	for (int i = 0; i < lengthof(PrimitiveTypeStr); ++i) {
		if (PrimitiveTypeStr[i] == nullptr) {
			continue;
		}
		if (token == PrimitiveTypeStr[i]) {
			return (PrimitiveType)i;
		}
	}
	return PrimitiveType::None;
}

bool parseLayout(TokenIterator& tok, Layout& layout) {
	if (!tok.hasNext()) {
		return false;
	}

	std::string token = tok.next();
	if (token != "(") {
		Log::warn("Unexpected layout syntax - expected {, got %s", token.c_str());
		return false;
	}
	do {
		if (!tok.hasNext()) {
			return false;
		}
		token = tok.next();
		Log::trace("token: %s", token.c_str());
		if (token == ")") {
			break;
		}
		if (token == ",") {
			continue;
		}
		if (token == "std140") {
			layout.blockLayout = BlockLayout::std140;
		} else if (token == "std430") {
			layout.blockLayout = BlockLayout::std430;
		} else if (token == "location") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.location = core::string::toInt(tok.next());
		} else if (token == "offset") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.offset = core::string::toInt(tok.next());
		} else if (token == "compontents") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.components = core::string::toInt(tok.next());
		} else if (token == "index") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.index = core::string::toInt(tok.next());
		} else if (token == "binding") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.binding = core::string::toInt(tok.next());
		} else if (token == "xfb_buffer") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackBuffer = core::string::toInt(tok.next());
		} else if (token == "xfb_offset") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackOffset = core::string::toInt(tok.next());
		} else if (token == "vertices") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.tesselationVertices = core::string::toInt(tok.next());
		} else if (token == "max_vertices") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.maxGeometryVertices = core::string::toInt(tok.next());
		} else if (token == "origin_upper_left") {
			layout.originUpperLeft = true;
		} else if (token == "pixel_center_integer") {
			layout.pixelCenterInteger = true;
		} else if (token == "early_fragment_tests") {
			layout.earlyFragmentTests = true;
		} else if (token == "primitive_type") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.primitiveType = layoutPrimitiveType(tok.next());
		} else if (token == "local_size_x") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.localSize.x = core::string::toInt(tok.next());
		} else if (token == "local_size_y") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.localSize.y = core::string::toInt(tok.next());
		} else if (token == "local_size_z") {
			core_assert_always(tok.hasNext() && tok.next() == "=");
			if (!tok.hasNext()) {
				return false;
			}
			layout.localSize.z = core::string::toInt(tok.next());
		} else {
			LayoutImageFormat format = util::getLayoutImageFormat(token, tok.line());
			if (format != LayoutImageFormat::Max) {
				layout.imageFormat = format;
			} else {
				Log::warn("Unknown token given for layout: %s (line %i)", token.c_str(), tok.line());
			}
		}
	} while (token != ")");

	return true;
}

bool parse(ShaderStruct& shaderStruct, const std::string& shaderFile, const std::string& buffer, bool vertex) {
	bool uniformBlock = false;

	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	std::stringstream f(buffer);
	simplecpp::TokenList rawtokens(f, files, shaderFile, &outputList);
	std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);
	simplecpp::TokenList output(files);
	simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList);

	simplecpp::Location loc(files);
	std::stringstream comment;
	UniformBlock block;

	TokenIterator tok;
	tok.init(&output);

	Layout layout;
	bool hasLayout = false;
	while (tok.hasNext()) {
		const std::string token = tok.next();
		Log::trace("token: %s", token.c_str());
		std::vector<Variable>* v = nullptr;
		if (token == "$in") {
			if (vertex) {
				v = &shaderStruct.attributes;
			} else {
				// TODO: use this to validate that each $out of the vertex shader has a $in in the fragment shader
				//v = &_shaderStruct.varyings;
			}
		} else if (token == "$out") {
			if (vertex) {
				v = &shaderStruct.varyings;
			} else {
				v = &shaderStruct.outs;
			}
		} else if (token == "layout") {
			// there can be multiple layouts per definition since GL 4.2 (or ARB_shading_language_420pack)
			// that's why we only reset the layout after we finished parsing the variable and/or the
			// uniform buffer. The last defined value for the mutually-exclusive qualifiers or for numeric
			// qualifiers prevails.
			layout = Layout();
			hasLayout = true;
			if (!parseLayout(tok, layout)) {
				Log::warn("Could not parse layout");
			}
		} else if (token == "buffer") {
			Log::warn("SSBO not supported");
		} else if (token == "uniform") {
			v = &shaderStruct.uniforms;
		} else if (uniformBlock) {
			if (token == "}") {
				uniformBlock = false;
				hasLayout = false;
				Log::trace("End of uniform block: %s", block.name.c_str());
				shaderStruct.uniformBlocks.push_back(block);
				core_assert_always(tok.next() == ";");
			} else {
				tok.prev();
			}
		}

		if (v == nullptr && !uniformBlock) {
			continue;
		}

		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get type");
			return false;
		}
		std::string type = tok.next();
		Log::trace("token: %s", type.c_str());
		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get variable name for type %s", type.c_str());
			return false;
		}
		while (type == "highp" || type == "mediump" || type == "lowp" || type == "precision") {
			Log::trace("token: %s", type.c_str());
			if (!tok.hasNext()) {
				Log::error("Failed to parse the shader, could not get type");
				return false;
			}
			type = tok.next();
		}
		std::string name = tok.next();
		Log::trace("token: %s", name.c_str());
		// uniform block
		if (name == "{") {
			block.name = type;
			block.members.clear();
			Log::trace("Found uniform block: %s", type.c_str());
			uniformBlock = true;
			continue;
		}
		const Variable::Type typeEnum = util::getType(type, tok.line());
		const bool isArray = tok.peekNext() == "[";
		int arraySize = 0;
		if (isArray) {
			tok.next();
			const std::string& number = tok.next();
			core_assert_always(tok.next() == "]");
			core_assert_always(tok.next() == ";");
			arraySize = core::string::toInt(number);
			if (arraySize == 0) {
				arraySize = -1;
				Log::warn("Could not determine array size for %s (%s)", name.c_str(), number.c_str());
			}
		}
		// TODO: multi dimensional arrays are only supported in glsl >= 5.50
		if (uniformBlock) {
			block.members.push_back(Variable{typeEnum, name, arraySize});
		} else {
			auto findIter = std::find_if(v->begin(), v->end(), [&] (const Variable& var) {return var.name == name;});
			if (findIter == v->end()) {
				v->push_back(Variable{typeEnum, name, arraySize});
				if (hasLayout) {
					shaderStruct.layouts[name] = layout;
					hasLayout = false;
				}
			} else if (typeEnum != findIter->type) {
				// TODO: check layout differences
				Log::error("Found duplicate variable %s (%s versus %s)",
					name.c_str(), util::resolveTypes(findIter->type).ctype, util::resolveTypes(typeEnum).ctype);
				return false;
			}
		}
	}
	if (uniformBlock) {
		Log::error("Parsing error - still inside a uniform block");
		return false;
	}
	return true;
}

}
