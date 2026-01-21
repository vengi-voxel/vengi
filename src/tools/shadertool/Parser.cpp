/**
 * @file Parser.cpp
 * @brief Shader parser implementation for layout and structure parsing
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
static_assert(core::enumVal(video::Primitive::Max) == lengthof(PrimitiveStr),
	"PrimitiveStr size doesn't match video::Primitive enum");

static video::Primitive layoutPrimitiveType(const core::String& token) {
	for (int i = 0; i < lengthof(PrimitiveStr); ++i) {
		if (token == PrimitiveStr[i]) {
			return static_cast<video::Primitive>(i);
		}
	}
	return video::Primitive::Max;
}

static bool parseLayoutParameter(TokenIterator& tok, Layout& layout) {
	const core::String token = tok.next();
	Log::trace("layout token: %s", token.c_str());
	
	if (token == ")") {
		return false; // Signal end of layout parameters
	}
	
	if (token == ",") {
		return true; // Continue parsing
	}
	
	// Handle layout parameters with assignment
	struct LayoutParam {
		const char* name;
		bool requiresAssignment;
		std::function<bool(const core::String&, Layout&)> handler;
	};
	
	static const LayoutParam layoutParams[] = {
		{"std140", false, [](const core::String&, Layout& l) { l.blockLayout = BlockLayout::std140; return true; }},
		{"std430", false, [](const core::String&, Layout& l) { l.blockLayout = BlockLayout::std140; return true; }},
		{"origin_upper_left", false, [](const core::String&, Layout& l) { l.originUpperLeft = true; return true; }},
		{"pixel_center_integer", false, [](const core::String&, Layout& l) { l.pixelCenterInteger = true; return true; }},
		{"early_fragment_tests", false, [](const core::String&, Layout& l) { l.earlyFragmentTests = true; return true; }},
		
		{"location", true, [&tok](const core::String&, Layout& l) {
			l.location = core::string::toInt(tok.next());
			return true;
		}},
		{"offset", true, [&tok](const core::String&, Layout& l) {
			l.offset = core::string::toInt(tok.next());
			return true;
		}},
		{"binding", true, [&tok](const core::String&, Layout& l) {
			l.binding = core::string::toInt(tok.next());
			return true;
		}},
		{"index", true, [&tok](const core::String&, Layout& l) {
			l.index = core::string::toInt(tok.next());
			return true;
		}},
		{"xfb_buffer", true, [&tok](const core::String&, Layout& l) {
			l.transformFeedbackBuffer = core::string::toInt(tok.next());
			return true;
		}},
		{"xfb_offset", true, [&tok](const core::String&, Layout& l) {
			l.transformFeedbackOffset = core::string::toInt(tok.next());
			return true;
		}},
		{"vertices", true, [&tok](const core::String&, Layout& l) {
			l.tesselationVertices = core::string::toInt(tok.next());
			return true;
		}},
		{"max_vertices", true, [&tok](const core::String&, Layout& l) {
			l.maxGeometryVertices = core::string::toInt(tok.next());
			return true;
		}},
		{"local_size_x", true, [&tok](const core::String&, Layout& l) {
			l.localSize.x = core::string::toInt(tok.next());
			return true;
		}},
		{"local_size_y", true, [&tok](const core::String&, Layout& l) {
			l.localSize.y = core::string::toInt(tok.next());
			return true;
		}},
		{"local_size_z", true, [&tok](const core::String&, Layout& l) {
			l.localSize.z = core::string::toInt(tok.next());
			return true;
		}},
		{"primitive_type", true, [&tok](const core::String&, Layout& l) {
			l.primitiveType = layoutPrimitiveType(tok.next());
			return true;
		}}
	};
	
	// Check for known layout parameters
	for (const auto& param : layoutParams) {
		if (token == param.name) {
			if (param.requiresAssignment) {
				if (!tok.hasNext() || tok.next() != "=") {
					Log::error("Expected = for %s", param.name);
					return false;
				}
				if (!tok.hasNext()) {
					Log::error("Missing value for %s", param.name);
					return false;
				}
			}
			return param.handler(token, layout);
		}
	}
	
	// Handle typo in "components" (present in original code)
	if (token == "compontents") {
		if (!tok.hasNext() || tok.next() != "=") {
			Log::error("Expected = for components");
			return false;
		}
		if (!tok.hasNext()) {
			return false;
		}
		layout.components = core::string::toInt(tok.next());
		return true;
	}
	
	// Check for image format
	video::ImageFormat format = util::getImageFormat(token, tok.line());
	if (format != video::ImageFormat::Max) {
		layout.imageFormat = format;
		return true;
	}
	
	// Check for primitive type without explicit primitive_type= prefix
	video::Primitive primitiveType = layoutPrimitiveType(token);
	if (primitiveType != video::Primitive::Max) {
		layout.primitiveType = primitiveType;
		return true;
	}
	
	Log::warn("Warning in %s:%i:%i. Unknown layout qualifier: %s", 
		tok.file(), tok.line(), tok.col(), token.c_str());
	return true;
}

static bool parseLayout(TokenIterator& tok, Layout& layout) {
	if (!tok.hasNext()) {
		return false;
	}

	const core::String token = tok.next();
	if (token != "(") {
		Log::warn("Warning in %s:%i:%i. Unexpected layout syntax - expected (, got %s", 
			tok.file(), tok.line(), tok.col(), token.c_str());
		return false;
	}
	
	do {
		if (!tok.hasNext()) {
			Log::error("Unexpected end of layout qualifiers");
			return false;
		}
		
		if (!parseLayoutParameter(tok, layout)) {
			break; // Reached closing parenthesis
		}
	} while (true);
	
	return true;
}

static bool parseArraySpecifier(TokenIterator& tok, int& arraySize) {
	arraySize = 0;
	
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

static bool validatePreprocessorDirective(TokenIterator& rawtok) {
	bool preprocessorError = false;
	
	while (rawtok.hasNext()) {
		const core::String token = rawtok.next();
		if (rawtok.op() != '#') {
			continue;
		}
		
		if (!rawtok.hasNext()) {
			Log::error("Error in %s:%i:%i. Incomplete preprocessor directive",
				rawtok.file(), rawtok.line(), rawtok.col());
			preprocessorError = true;
			continue;
		}
		
		const core::String directive = rawtok.next();
		if (directive == "ifdef" || directive == "ifndef" || 
			directive == "define" || directive == "if") {
			
			if (!rawtok.hasNext()) {
				Log::error("Error in %s:%i:%i. Incomplete %s directive",
					rawtok.file(), rawtok.line(), rawtok.col(), directive.c_str());
				preprocessorError = true;
				continue;
			}
			
			const core::String& preprocessorToken = rawtok.next();
			if (core::string::contains(preprocessorToken.c_str(), "_")) {
				Log::warn("Warning in %s:%i:%i. Preprocessor token with underscore (may not be supported by all drivers): %s",
					rawtok.file(), rawtok.line(), rawtok.col(), preprocessorToken.c_str());
				Log::warn("If this is a shader cvar define, consider removing the underscore");
			}
		}
	}
	
	return !preprocessorError;
}

bool parse(const core::String& filename, ShaderStruct& shaderStruct, 
		   const core::String& shaderFile, const core::String& buffer, bool vertex) {
	
	shaderStruct.filename = shaderFile;
	shaderStruct.name = shaderFile;
	
	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	
	simplecpp::TokenList rawtokens(buffer.c_str(), buffer.size(), 
		files, filename.c_str(), &outputList);
	
	std::map<std::string, simplecpp::TokenList*> included = 
		simplecpp::load(rawtokens, files, dui, &outputList);
	
	simplecpp::TokenList output(files);
	std::list<simplecpp::MacroUsage> macroUsage;
	simplecpp::preprocess(output, rawtokens, files, included, 
		dui, &outputList, &macroUsage);
	
	TokenIterator rawtok;
	rawtok.init(&rawtokens);
	
	if (!validatePreprocessorDirective(rawtok)) {
		return false;
	}
	
	TokenIterator tok;
	tok.init(&output);
	
	Layout layout;
	bool hasLayout = false;
	
	BufferBlock uniformBuffer;
	bool uniformBufferActive = false;
	
	BufferBlock shaderStorageBuffer;
	bool shaderStorageBufferActive = false;
	bool shaderStorageBufferFound = false;
	
	while (tok.hasNext()) {
		const core::String token = tok.next();
		Log::trace("main token: %s", token.c_str());
		
		core::List<Variable>* targetList = nullptr;
		
		
		if (token == "$in") {
			targetList = vertex ? &shaderStruct.attributes : nullptr;
			// TODO: Use for validation between vertex and fragment shaders
		} else if (token == "$out") {
			targetList = vertex ? &shaderStruct.varyings : &shaderStruct.outs;
		} else if (token == "$constant") {
			if (!parseConstant(tok, shaderStruct.constants)) {
				return false;
			}
			continue;
		} else if (token == "layout") {
			layout = Layout(); // Reset layout
			hasLayout = true;
			if (!parseLayout(tok, layout)) {
				Log::warn("Warning in %s:%i:%i. Failed to parse layout", 
					tok.file(), tok.line(), tok.col());
			}
			continue;
		} else if (token == "buffer") {
			shaderStorageBufferFound = true;
			continue;
		} else if (token == "uniform") {
			targetList = &shaderStruct.uniforms;
		} else if (hasLayout && token == "in") {
			shaderStruct.in.layout = layout;
			hasLayout = false;
			continue;
		} else if (hasLayout && token == "out") {
			shaderStruct.out.layout = layout;
			hasLayout = false;
			continue;
		} else if (uniformBufferActive) {
			if (token == "}") {
				uniformBufferActive = false;
				if (hasLayout) {
					uniformBuffer.layout = layout;
					hasLayout = false;
				}
				shaderStruct.uniformBlocks.insert(uniformBuffer);
				
				if (tok.next() != ";") {
					Log::error("Missing ; after uniform block: %s (%s:%i)", 
						uniformBuffer.name.c_str(), tok.file(), tok.line());
					return false;
				}
				continue;
			}
			tok.prev(); 
		} else if (shaderStorageBufferActive) {
			if (token == "}") {
				shaderStorageBufferActive = false;
				if (hasLayout) {
					shaderStorageBuffer.layout = layout;
					hasLayout = false;
				}
				
				if (tok.hasNext()) {
					const core::String& nextToken = tok.next();
					if (nextToken != ";") {
						shaderStorageBuffer.name = shaderStorageBuffer.name + "_" + nextToken;
						if (!tok.hasNext() || tok.next() != ";") {
							Log::error("Missing ; after buffer block instance: %s (%s:%i)", 
								shaderStorageBuffer.name.c_str(), tok.file(), tok.line());
							return false;
						}
					} else {
						tok.prev(); 
					}
				}
				
				shaderStruct.bufferBlocks.insert(shaderStorageBuffer);
				
				if (tok.next() != ";") {
					Log::error("Missing ; after buffer block: %s (%s:%i)", 
						shaderStorageBuffer.name.c_str(), tok.file(), tok.line());
					return false;
				}
				continue;
			}
			tok.prev(); 
		}
		
		if (!targetList && !uniformBufferActive && !shaderStorageBufferActive) {
			continue;
		}
		
		if (!tok.hasNext()) {
			Log::error("Error in %s:%i:%i. Expected type", 
				tok.file(), tok.line(), tok.col());
			return false;
		}
		
		core::String type = tok.next();
		if (!parseTypeQualifiers(tok, type)) {
			return false;
		}
		
		if (!tok.hasNext()) {
			Log::error("Error in %s:%i:%i. Expected identifier for type %s", 
				tok.file(), tok.line(), tok.col(), type.c_str());
			return false;
		}
		
		core::String name = tok.next();
		
		if (name == "{") {
			if (shaderStorageBufferFound) {
				shaderStorageBuffer.name = type;
				shaderStorageBuffer.members.clear();
				shaderStorageBufferActive = true;
				shaderStorageBufferFound = false;
			} else {
				uniformBuffer.name = type;
				uniformBuffer.members.clear();
				uniformBufferActive = true;
			}
			continue;
		}
		
		const Variable::Type typeEnum = util::getType(type, tok.line());
		int arraySize = 0;
		
		if (parseArraySpecifier(tok, arraySize)) {
			if (tok.next() != ";") {
				Log::error("Missing ; for array %s (%s:%i)", 
					name.c_str(), tok.file(), tok.line());
				return false;
			}
		} else {
			if (tok.next() != ";") {
				Log::error("Missing ; for variable %s (%s:%i)", 
					name.c_str(), tok.file(), tok.line());
				return false;
			}
		}
		
		if (uniformBufferActive) {
			uniformBuffer.members.insert(Variable{typeEnum, name, arraySize});
		} else if (shaderStorageBufferActive) {
			shaderStorageBuffer.members.insert(Variable{typeEnum, name, arraySize});
		} else if (targetList) {

			bool duplicate = false;
			for (const auto& var : *targetList) {
				if (var.name == name) {
					duplicate = true;
					if (var.type != typeEnum) {
						Log::error("Error in %s:%i:%i. Duplicate variable %s with different type", 
							tok.file(), tok.line(), tok.col(), name.c_str());
						return false;
					}
					break;
				}
			}
			
			if (!duplicate) {
				targetList->insert(Variable{typeEnum, name, arraySize});
				if (hasLayout) {
					shaderStruct.layouts.put(name, layout);
					hasLayout = false;
				}
			}
		}
	}
	
	// Validate parsing state
	if (uniformBufferActive) {
		Log::error("Parsing error - unterminated uniform block");
		return false;
	}
	
	if (shaderStorageBufferActive) {
		Log::error("Parsing error - unterminated buffer block");
		return false;
	}
	
	return true;
}

// Helper function for parsing constants
static bool parseConstant(TokenIterator& tok, core::Map<core::String, core::String>& constants) {
	if (!tok.hasNext()) {
		Log::error("Expected constant name");
		return false;
	}
	
	const core::String name = tok.next();
	
	if (!tok.hasNext()) {
		Log::error("Expected constant value for %s", name.c_str());
		return false;
	}
	
	const core::String value = tok.next();
	
	auto it = constants.find(name);
	if (it != constants.end()) {
		if (it->value != value) {
			Log::warn("Warning in %s:%i:%i. Constant %s redefined with different value (%s vs %s)", 
				tok.file(), tok.line(), tok.col(), name.c_str(), it->value.c_str(), value.c_str());
		}
	} else {
		constants.put(name, value);
	}
	
	return true;
}

} // namespace shadertool
