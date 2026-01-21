/**
 * @file
 */

#include "Parser.h"
#include "TokenIterator.h"
#include "core/ArrayLength.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "Util.h"

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
			// std430 is used for SSBOs (shader storage buffers) in compute shaders
			layout.blockLayout = BlockLayout::std140; // Treat like std140 for now
		} else if (token == "location") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for location");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.location = core::string::toInt(tok.next());
		} else if (token == "offset") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for offset");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.offset = core::string::toInt(tok.next());
		} else if (token == "component") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for component");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.components = core::string::toInt(tok.next());
		} else if (token == "index") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for index");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.index = core::string::toInt(tok.next());
		} else if (token == "binding") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for binding");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.binding = core::string::toInt(tok.next());
		} else if (token == "xfb_buffer") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for xfb_buffer");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackBuffer = core::string::toInt(tok.next());
		} else if (token == "xfb_offset") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for xfb_offset");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackOffset = core::string::toInt(tok.next());
		} else if (token == "vertices") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for vertices");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.tesselationVertices = core::string::toInt(tok.next());
		} else if (token == "max_vertices") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected = for max_vertices");
				return false;
			}
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
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected =");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.primitiveType = layoutPrimitiveType(tok.next());
		} else if (token == "local_size_x") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected =");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.localSize.x = core::string::toInt(tok.next());
		} else if (token == "local_size_y") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected =");
				return false;
			}
			if (!tok.hasNext()) {
				return false;
			}
			layout.localSize.y = core::string::toInt(tok.next());
		} else if (token == "local_size_z") {
			if (!tok.hasNext() || tok.next() != "=") {
				Log::error("Expected =");
				return false;
			}
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

static bool validatePreprocessorDirective(TokenIterator& rawtok) {
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
	return !preprocessorError;
}

static bool parseArraySpecifier(TokenIterator& tok, int& arraySize) {
	if (tok.peekNext() != "[") {
		return false; // Not an array
	}
	
	tok.next(); // Consume '['
	
	if (tok.peekNext() == "]") {
		// Dynamic sized array (SSBO)
		arraySize = -1;
	} else {
		const core::String& sizeStr = tok.next();
		arraySize = core::string::toInt(sizeStr);
		if (arraySize <= 0) {
			arraySize = -1;
			Log::warn("Warning in %s:%i:%i. Could not determine array size (%s)", 
				tok.file(), tok.line(), tok.col(), sizeStr.c_str());
		}
	}
	
	if (tok.next() != "]") {
		Log::error("Missing ] for array declaration");
		return false;
	}
	
	return true;
}

static bool parseTypeQualifiers(TokenIterator& tok, core::String& type) {
	// Skip type qualifiers that don't affect parsing
	// https://www.khronos.org/opengl/wiki/Type_Qualifier_(GLSL)
	static const core::String qualifiers[] = {
		"uniform", "highp", "mediump", "lowp", 
		"precision", "flat", "noperspective", "smooth"
	};
	
	while (true) {
		bool isQualifier = false;
		for (const auto& qual : qualifiers) {
			if (type == qual) {
				isQualifier = true;
				break;
			}
		}
		
		if (!isQualifier) {
			break;
		}
		
		Log::trace("skipping qualifier: %s", type.c_str());
		
		if (!tok.hasNext()) {
			Log::error("Error in %s:%i:%i. Expected type after qualifier", 
				tok.file(), tok.line(), tok.col());
			return false;
		}
		
		type = tok.next();
	}
	
	return true;
}

bool parse(const core::String& filename, ShaderStruct& shaderStruct, const core::String& shaderFile, const core::String& buffer, bool vertex) {
	shaderStruct.filename = shaderFile;
	shaderStruct.name = shaderFile;
	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	simplecpp::TokenList rawtokens(buffer.c_str(), buffer.size(), files, filename.c_str(), &outputList);
	std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);

	simplecpp::TokenList output(files);
	std::list<simplecpp::MacroUsage> macroUsage;
	simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList, &macroUsage);

	TokenIterator rawtok;
	rawtok.init(&rawtokens);

	if (!validatePreprocessorDirective(rawtok)) {
		return false;
	}

	simplecpp::Location loc(files);

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
			auto it = shaderStruct.constants.find(varname);
			if (it != shaderStruct.constants.end()) {
				if (it->value != varvalue) {
					Log::warn("Warning in %s:%i:%i. Could not register constant %s with value %s (duplicate has value %s)", tok.file(), tok.line(), tok.col(),
							varname.c_str(), varvalue.c_str(), it->value.c_str());
				}
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
					uniformBuffer.layout = layout;
				}
				Log::trace("End of uniform block: %s", uniformBuffer.name.c_str());
				shaderStruct.uniformBlocks.insert(uniformBuffer);
				if (tok.next() != ";") {
					Log::error("Missing ; in uniform block: %s (%s:%i)", uniformBuffer.name.c_str(), tok.file(), tok.line());
					return false;
				}
			} else {
				tok.prev();
			}
		} else if (shaderStorageBufferActive) {
			if (token == "}") {
				shaderStorageBufferActive = false;
				if (hasLayout) {
					shaderStorageBuffer.layout = layout;
				}
				Log::trace("End of buffer block: %s", shaderStorageBuffer.name.c_str());

				// Check if there's an instance name after the closing brace
				if (tok.hasNext()) {
					const core::String &instanceName = tok.next();
					if (instanceName != ";") {
						// Store instance name in the buffer block
						shaderStorageBuffer.name = shaderStorageBuffer.name + "_" + instanceName;
						Log::trace("Buffer block instance name: %s", instanceName.c_str());
						if (!tok.hasNext() || tok.next() != ";") {
							Log::error("Missing ; for storage buffer block instance: %s (%s:%i)", shaderStorageBuffer.name.c_str(), tok.file(), tok.line());
							return false;
						}
					} else {
						tok.prev(); // Put ; back
					}
				}

				shaderStruct.bufferBlocks.insert(shaderStorageBuffer);
				if (tok.next() != ";") {
					Log::error("Missing ; for storage buffer block: %s (%s:%i)", shaderStorageBuffer.name.c_str(), tok.file(), tok.line());
					return false;
				}
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
		
		if (!parseTypeQualifiers(tok, type)) {
			return false;
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
		int arraySize = 0;
		if (parseArraySpecifier(tok, arraySize)) {
			if (tok.next() != ";") {
				Log::error("Missing ; for array %s (%s:%i)", name.c_str(), tok.file(), tok.line());
				return false;
			}
		} else {
			if (tok.next() != ";") {
				Log::error("Missing ; for variable %s (%s:%i)", name.c_str(), tok.file(), tok.line());
				return false;
			}
		}
		// TODO: multi dimensional arrays are only supported in glsl >= 5.50
		if (uniformBufferActive) {
			uniformBuffer.members.insert(Variable{typeEnum, name, arraySize});
		} else if (shaderStorageBufferActive) {
			shaderStorageBuffer.members.insert(Variable{typeEnum, name, arraySize});
		} else {
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
