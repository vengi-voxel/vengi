/**
 * @file
 */

#include "Parser.h"
#include "TokenIterator.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "video/Shader.h"
#include "Generator.h"
#include "Parser.h"
#include "Util.h"
#include <sstream>
#include <algorithm>

namespace shadertool {

static const char* PrimitiveStr[] {
	"points",
	"lines",
	"lines_adjacency",
	"triangles",
	"triangles_adjacency",
	"line_strip",
	"triangle_strip"
};
static_assert(lengthof(PrimitiveStr) == core::enumVal(video::Primitive::Max), "PrimitiveStr doesn't match enum");

static video::Primitive layoutPrimitiveType(const core::String& token) {
	for (int i = 0; i < lengthof(PrimitiveStr); ++i) {
		if (token == PrimitiveStr[i]) {
			return (video::Primitive)i;
		}
	}
	return video::Primitive::Max;
}

static bool parseLayout(TokenIterator& tok, Layout& layout) {
	if (!tok.hasNext()) {
		return false;
	}

	core::String token = tok.next();
	if (token != "(") {
		Log::warn("Warning in %s:%i:%i. Unexpected layout syntax - expected {, got %s", tok.file(), tok.line(), tok.col(), token.c_str());
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
			video::ImageFormat format = util::getImageFormat(token, tok.line());
			if (format != video::ImageFormat::Max) {
				layout.imageFormat = format;
			} else {
				video::Primitive primitiveType = layoutPrimitiveType(token);
				if (primitiveType != video::Primitive::Max) {
					layout.primitiveType = primitiveType;
				} else {
					Log::warn("Warning in %s:%i:%i. Unknown token given for layout: %s", tok.file(), tok.line(), tok.col(), token.c_str());
				}
			}
		}
	} while (token != ")");

	return true;
}

bool parse(const core::String& filename, ShaderStruct& shaderStruct, const core::String& shaderFile, const core::String& buffer, bool vertex) {
	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	std::stringstream f(buffer.c_str());
	simplecpp::TokenList rawtokens(f, files, filename.c_str(), &outputList);
	std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);

	simplecpp::TokenList output(files);
	std::list<simplecpp::MacroUsage> macroUsage;
	simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList, &macroUsage);

	TokenIterator rawtok;
	rawtok.init(&rawtokens);

	bool preprocessorError = false;

	while (rawtok.hasNext()) {
		core::String token = rawtok.next();
		if (rawtok.op() != '#') {
			continue;
		}
		if (!rawtok.hasNext()) {
			Log::error("Error in %s:%i:%i. Found preprocessor directive, but no further token",
					rawtok.file(), rawtok.line(), rawtok.col());
			preprocessorError = true;
		}
		token = rawtok.next();
		if (token == "ifdef" || token == "ifndef" || token == "define" || token == "if") {
			if (!rawtok.hasNext()) {
				Log::error("Error in %s:%i:%i. Found preprocessor directive, but no further token",
						rawtok.file(), rawtok.line(), rawtok.col());
				preprocessorError = true;
			}
			const core::String& preprocessor = rawtok.next();
			if (core::string::contains(preprocessor.c_str(), "_")) {
				Log::warn("Warning in %s:%i:%i. Found preprocessor token with _ - some drivers doesn't support this: %s",
						rawtok.file(), rawtok.line(), rawtok.col(), preprocessor.c_str());
				Log::warn("If this is a shader cvar define, just remove the _");
			}
		}
	}
	if (preprocessorError) {
		return false;
	}

	simplecpp::Location loc(files);
	std::stringstream comment;

	BufferBlock uniformBuffer;
	bool uniformBufferActive = false;

	BufferBlock shaderStorageBuffer;
	bool shaderStorageBufferActive = false;
	bool shaderStorageBufferFound = false;

	TokenIterator tok;
	tok.init(&output);

	Layout layout;
	bool hasLayout = false;
	while (tok.hasNext()) {
		const core::String token = tok.next();
		Log::trace("token: %s", token.c_str());
		core::List<Variable>* v = nullptr;
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
		} else if (token == "$constant") {
			if (!tok.hasNext()) {
				return false;
			}
			const core::String varname = tok.next();
			if (!tok.hasNext()) {
				return false;
			}
			const core::String varvalue = tok.next();
			if (shaderStruct.constants.find(varname) != shaderStruct.constants.end()) {
				Log::warn("Warning in %s:%i:%i. Could not register constant %s with value %s (duplicate)", tok.file(), tok.line(), tok.col(),
						varname.c_str(), varvalue.c_str());
			} else {
				shaderStruct.constants.put(varname, varvalue);
			}
		} else if (token == "layout") {
			// there can be multiple layouts per definition since GL 4.2 (or ARB_shading_language_420pack)
			// that's why we only reset the layout after we finished parsing the variable and/or the
			// uniform buffer. The last defined value for the mutually-exclusive qualifiers or for numeric
			// qualifiers prevails.
			layout = Layout();
			hasLayout = true;
			if (!parseLayout(tok, layout)) {
				Log::warn("Warning in %s:%i:%i. Could not parse layout", tok.file(), tok.line(), tok.col());
			}
		} else if (token == "buffer") {
			shaderStorageBufferFound = true;
		} else if (token == "uniform") {
			v = &shaderStruct.uniforms;
		} else if (hasLayout && token == "in") {
			shaderStruct.in.layout = layout;
		} else if (hasLayout && token == "out") {
			shaderStruct.out.layout = layout;
		} else if (uniformBufferActive) {
			if (token == "}") {
				uniformBufferActive = false;
				if (hasLayout) {
					hasLayout = false;
					uniformBuffer.layout = layout;
				}
				Log::trace("End of uniform block: %s", uniformBuffer.name.c_str());
				shaderStruct.uniformBlocks.insert(uniformBuffer);
				core_assert_always(tok.next() == ";");
			} else {
				tok.prev();
			}
		} else if (shaderStorageBufferActive) {
			if (token == "}") {
				shaderStorageBufferActive = false;
				if (hasLayout) {
					hasLayout = false;
					shaderStorageBuffer.layout = layout;
				}
				Log::trace("End of buffer block: %s", shaderStorageBuffer.name.c_str());
				shaderStruct.bufferBlocks.insert(shaderStorageBuffer);
				core_assert_always(tok.next() == ";");
			} else {
				tok.prev();
			}
		}

		if (v == nullptr && !uniformBufferActive && !shaderStorageBufferActive && !shaderStorageBufferFound) {
			continue;
		}

		if (!tok.hasNext()) {
			Log::error("Error in %s:%i:%i. Failed to parse the shader, could not get type", tok.file(), tok.line(), tok.col());
			return false;
		}
		core::String type = tok.next();
		Log::trace("token: %s", type.c_str());
		if (!tok.hasNext()) {
			Log::error("Error in %s:%i:%i. Failed to parse the shader, could not get variable name for type %s", tok.file(), tok.line(), tok.col(), type.c_str());
			return false;
		}
		while (type == "highp" || type == "mediump" || type == "lowp" || type == "precision") {
			Log::trace("token: %s", type.c_str());
			if (!tok.hasNext()) {
				Log::error("Error in %s:%i:%i. Failed to parse the shader, could not get type", tok.file(), tok.line(), tok.col());
				return false;
			}
			type = tok.next();
		}
		core::String name = tok.next();
		Log::trace("token: %s", name.c_str());
		// uniform block or buffer block
		if (name == "{") {
			if (shaderStorageBufferFound) {
				shaderStorageBuffer.name = type;
				shaderStorageBuffer.members.clear();
				Log::trace("Found uniform or buffer block: %s", type.c_str());
				shaderStorageBufferActive = true;
				shaderStorageBufferFound = false;
			} else {
				uniformBuffer.name = type;
				uniformBuffer.members.clear();
				Log::trace("Found uniform or buffer block: %s", type.c_str());
				uniformBufferActive = true;
			}
			continue;
		}
		const Variable::Type typeEnum = util::getType(type, tok.line());
		const bool isArray = tok.peekNext() == "[";
		int arraySize = 0;
		if (isArray) {
			tok.next();
			if (tok.peekNext() == "]") {
				// dynamic sized array (ssbo)
				arraySize = -1;
			} else {
				const core::String& number = tok.next();
				arraySize = core::string::toInt(number);
				if (arraySize == 0) {
					arraySize = -1;
					Log::warn("Warning in %s:%i:%i. Could not determine array size for %s (%s)", tok.file(), tok.line(), tok.col(), name.c_str(), number.c_str());
				}
			}
			core_assert_always(tok.next() == "]");
			core_assert_always(tok.next() == ";");
		}
		// TODO: multi dimensional arrays are only supported in glsl >= 5.50
		if (uniformBufferActive) {
			uniformBuffer.members.insert(Variable{typeEnum, name, arraySize});
		} else if (!shaderStorageBufferActive) {
			bool found = false;
			for (auto i = v->begin(); i != v->end(); ++i) {
				if (i->value.name == name) {
					found = true;
					if (typeEnum != i->value.type) {
						// TODO: check layout differences
						Log::error("Error in %s:%i:%i. Found duplicate variable %s (%s versus %s)", tok.file(), tok.line(), tok.col(),
							name.c_str(), util::resolveTypes(i->value.type).ctype, util::resolveTypes(typeEnum).ctype);
						return false;
					}
					break;
				}
			}
			if (!found) {
				v->insert(Variable{typeEnum, name, arraySize});
				if (hasLayout) {
					shaderStruct.layouts.put(name, layout);
					hasLayout = false;
				}
			}
		}
	}
	if (uniformBufferActive) {
		Log::error("Parsing error - still inside a uniform block");
		return false;
	}
	if (shaderStorageBufferActive) {
		Log::error("Parsing error - still inside a buffer block");
		return false;
	}
	return true;
}

}
