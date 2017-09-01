/**
 * @file
 */

#include "ShaderTool.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "core/Process.h"
#include "core/GameConfig.h"
#include "video/Shader.h"
#include "video/Version.h"

const ShaderTool::Types ShaderTool::cTypes[] = {
	{ ShaderTool::Variable::DOUBLE,          1, "double",       Value,     "double" },
	{ ShaderTool::Variable::FLOAT,           1, "float",        Value,     "float" },
	{ ShaderTool::Variable::UNSIGNED_INT,    1, "uint32_t",     Value,     "uint" },
	{ ShaderTool::Variable::BOOL,            1, "bool",         Value,     "bool" },
	{ ShaderTool::Variable::INT,             1, "int32_t",      Value,     "int" },
	{ ShaderTool::Variable::BVEC2,           2, "glm::bvec2",   Reference, "bvec2" },
	{ ShaderTool::Variable::BVEC3,           3, "glm::bvec3",   Reference, "bvec3" },
	{ ShaderTool::Variable::BVEC4,           4, "glm::bvec4",   Reference, "bvec4" },
	{ ShaderTool::Variable::DVEC2,           2, "glm::dvec2",   Reference, "dvec2" },
	{ ShaderTool::Variable::DVEC3,           3, "glm::dvec3",   Reference, "dvec3" },
	{ ShaderTool::Variable::DVEC4,           4, "glm::dvec4",   Reference, "dvec4" },
	{ ShaderTool::Variable::UVEC2,           2, "glm::uvec2",   Reference, "uvec2" },
	{ ShaderTool::Variable::UVEC3,           3, "glm::uvec3",   Reference, "uvec3" },
	{ ShaderTool::Variable::UVEC4,           4, "glm::uvec4",   Reference, "uvec4" },
	{ ShaderTool::Variable::IVEC2,           2, "glm::ivec2",   Reference, "ivec2" },
	{ ShaderTool::Variable::IVEC3,           3, "glm::ivec3",   Reference, "ivec3" },
	{ ShaderTool::Variable::IVEC4,           4, "glm::ivec4",   Reference, "ivec4" },
	{ ShaderTool::Variable::VEC2,            2, "glm::vec2",    Reference, "vec2" },
	{ ShaderTool::Variable::VEC3,            3, "glm::vec3",    Reference, "vec3" },
	{ ShaderTool::Variable::VEC4,            4, "glm::vec4",    Reference, "vec4" },
	{ ShaderTool::Variable::MAT2,            1, "glm::mat2",    Reference, "mat2" },
	{ ShaderTool::Variable::MAT3,            1, "glm::mat3",    Reference, "mat3" },
	{ ShaderTool::Variable::MAT4,            1, "glm::mat4",    Reference, "mat4" },
	{ ShaderTool::Variable::MAT3X4,          1, "glm::mat3x4",  Reference, "mat3x4" },
	{ ShaderTool::Variable::MAT4X3,          1, "glm::mat4x3",  Reference, "mat4x3" },
	{ ShaderTool::Variable::SAMPLER1D,       1, "video::TextureUnit", Value,      "sampler1D" },
	{ ShaderTool::Variable::SAMPLER2D,       1, "video::TextureUnit", Value,      "sampler2D" },
	{ ShaderTool::Variable::SAMPLER2DARRAY,  1, "video::TextureUnit", Value,      "sampler2DArray" },
	{ ShaderTool::Variable::SAMPLER2DARRAYSHADOW, 1, "video::TextureUnit", Value, "sampler2DArrayShadow" },
	{ ShaderTool::Variable::SAMPLER3D,       1, "video::TextureUnit", Value,      "sampler3D" },
	{ ShaderTool::Variable::SAMPLERCUBEMAP,  1, "video::TextureUnit", Value,      "samplerCube" },
	{ ShaderTool::Variable::SAMPLER1DSHADOW, 1, "video::TextureUnit", Value,      "sampler1DShadow" },
	{ ShaderTool::Variable::SAMPLER2DSHADOW, 1, "video::TextureUnit", Value,      "sampler2DShadow" }
};

// TODO: generated sampler setters should use video::TextureUnit
// TODO: validate that each $out of the vertex shader has a $in in the fragment shader and vice versa
ShaderTool::ShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		core::App(filesystem, eventBus, timeProvider, 0) {
	init(ORGANISATION, "shadertool");
	static_assert(Variable::MAX == SDL_arraysize(cTypes), "mismatch in glsl types");
}

ShaderTool::~ShaderTool() {
}

static inline std::string convertName(const std::string& in, bool firstUpper) {
	std::string out;
	std::vector<std::string> nameParts;
	core::string::splitString(in, nameParts, "_");
	for (std::string& n : nameParts) {
		if (n.length() > 1 || nameParts.size() < 2) {
			if (!firstUpper) {
				firstUpper = true;
			} else {
				n[0] = SDL_toupper(n[0]);
			}
			out += n;
		}
	}
	if (out.empty()) {
		out = in;
	}
	return out;
}

std::string ShaderTool::uniformSetterPostfix(const ShaderTool::Variable::Type type, int amount) const {
	switch (type) {
	case Variable::MAX:
		return "";
	case Variable::FLOAT:
		if (amount > 1) {
			return "1fv";
		}
		return "f";
	case Variable::DOUBLE:
		if (amount > 1) {
			return "1dv";
		}
		return "d";
	case Variable::UNSIGNED_INT:
		if (amount > 1) {
			return "1uiv";
		}
		return "ui";
	case Variable::BOOL:
	case Variable::INT:
		if (amount > 1) {
			return "1iv";
		}
		return "i";
	case Variable::DVEC2:
	case Variable::BVEC2:
	case Variable::UVEC2:
	case Variable::VEC2:
		if (amount > 1) {
			return "Vec2v";
		}
		return "Vec2";
	case Variable::DVEC3:
	case Variable::BVEC3:
	case Variable::UVEC3:
	case Variable::VEC3:
		if (amount > 1) {
			return "Vec3v";
		}
		return "Vec3";
	case Variable::DVEC4:
	case Variable::BVEC4:
	case Variable::UVEC4:
	case Variable::VEC4:
		if (amount > 1) {
			return "Vec4v";
		}
		return "Vec4";
	case Variable::IVEC2:
		if (amount > 1) {
			return "Vec2v";
		}
		return "Vec2";
	case Variable::IVEC3:
		if (amount > 1) {
			return "Vec3v";
		}
		return "Vec3";
	case Variable::IVEC4:
		if (amount > 1) {
			return "Vec4v";
		}
		return "Vec4";
	case Variable::MAT3X4:
	case Variable::MAT4X3:
	case Variable::MAT2:
	case Variable::MAT3:
	case Variable::MAT4:
		if (amount > 1) {
			return "Matrixv";
		}
		return "Matrix";
	case Variable::SAMPLER1D:
	case Variable::SAMPLER2D:
	case Variable::SAMPLER3D:
	case Variable::SAMPLER1DSHADOW:
	case Variable::SAMPLER2DSHADOW:
	case Variable::SAMPLER2DARRAY:
	case Variable::SAMPLER2DARRAYSHADOW:
		if (amount > 1) {
			// https://www.opengl.org/wiki/Data_Type_%28GLSL%29#Opaque_arrays
			if (video::Shader::glslVersion < video::GLSLVersion::V400) {
				Log::warn("Sampler arrays are only allowed under special circumstances - don't do this for GLSL < 4.0");
			}
			// TODO: doesn't work yet, video::TextureUnit support is needed here
			return "1iv";
		}
		return "";
	case Variable::SAMPLERCUBEMAP:
		if (amount > 1) {
			return "1iv";
		}
		return "i";
	}
	return "";
}

int ShaderTool::getComponents(const ShaderTool::Variable::Type type) const {
	return cTypes[(int)type].components;
}

ShaderTool::Variable::Type ShaderTool::getType(const std::string& type) const {
	int max = std::enum_value(Variable::MAX);
	for (int i = 0; i < max; ++i) {
		if (type == cTypes[i].glsltype) {
			return cTypes[i].type;
		}
	}
	core_assert_msg(false, "Unknown type given: %s", type.c_str());
	return Variable::FLOAT;
}

#define USE_ALIGN_AS 0

/**
 * The size of each element in the array will be the size of the element type, rounded up to a multiple of the
 * size of a vec4. This is also the array’s alignment. The array’s size will be this rounded-up element’s size
 * times the number of elements in the array.
 * If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
 *
 * @note:
 * a float needs 4 bytes and it's 4 bytes aligned
 * a vec3 needs 12 bytes and it's 16 bytes aligned
 * a vec4 needs 16 bytes and it's 16 bytes aligned
 */
std::string ShaderTool::std140Align(const Variable& v) const {
#if USE_ALIGN_AS > 0
	// TODO: generete uniform buffer struct - enforce std140 layout
	// TODO: extract uniform blocks into aligned structs and generate methods to update them
	//       align them via GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT - use glBindBufferRange
	//       GL_MAX_UNIFORM_BLOCK_SIZE
	const Types& cType = cTypes[v.type];
	if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4
	 || cType.type == Variable::Type::DVEC2 || cType.type == Variable::Type::DVEC3 || cType.type == Variable::Type::DVEC4
	 || cType.type == Variable::Type::IVEC2 || cType.type == Variable::Type::IVEC3 || cType.type == Variable::Type::IVEC4
	 || cType.type == Variable::Type::BVEC2 || cType.type == Variable::Type::BVEC3 || cType.type == Variable::Type::BVEC4) {
		return "alignas(16) ";
	}
	if (cType.type == Variable::Type::FLOAT || cType.type == Variable::Type::DOUBLE) {
		return "alignas(4) ";
	}
#endif
	return "";
}

std::string ShaderTool::std140Padding(const Variable& v, int& padding) const {
#if USE_ALIGN_AS == 0
	const Types& cType = cTypes[v.type];
	if (cType.type == Variable::Type::VEC3
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::IVEC3
	 || cType.type == Variable::Type::BVEC3) {
		return "\t\tfloat _padding" + std::to_string(padding++) + ";\n";
	}
#endif
	return "";
}

size_t ShaderTool::std140Size(const Variable& v) const {
	const Types& cType = cTypes[v.type];
	int components = cType.components;
	int bytes = 4;
	if (cType.type == Variable::Type::DVEC2
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::DVEC4
	 || cType.type == Variable::Type::DOUBLE) {
		bytes = 8;
	}
	if (cType.type == Variable::Type::VEC2
	 || cType.type == Variable::Type::DVEC2
	 || cType.type == Variable::Type::IVEC2
	 || cType.type == Variable::Type::BVEC2) {
		components = 2;
	}
	if (cType.type == Variable::Type::VEC3
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::IVEC3
	 || cType.type == Variable::Type::BVEC3) {
		components = 4;
	}
	if (cType.type == Variable::Type::MAT2) {
		components = 4;
	} else if (cType.type == Variable::Type::MAT3) {
		components = 9; // FIXME
	} else if (cType.type == Variable::Type::MAT4) {
		components = 16;
	} else if (cType.type == Variable::Type::MAT3X4) {
		components = 16; // FIXME
	} else if (cType.type == Variable::Type::MAT4X3) {
		components = 16; // FIXME
	}
	if (v.arraySize > 0) {
		return components * bytes * v.arraySize;
	}
	return components * bytes;
}

void ShaderTool::generateSrc() {
	for (const auto& block : _shaderStruct.uniformBlocks) {
		Log::debug("Found uniform block %s with %i members", block.name.c_str(), int(block.members.size()));
	}
	for (const auto& v : _shaderStruct.uniforms) {
		Log::debug("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.attributes) {
		Log::debug("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.varyings) {
		Log::debug("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.outs) {
		Log::debug("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	const std::string& templateShader = core::App::getInstance()->filesystem()->load(_shaderTemplateFile);
	const std::string& templateUniformBuffer = core::App::getInstance()->filesystem()->load(_uniformBufferTemplateFile);
	std::string src(templateShader);
	std::string srcUb(templateUniformBuffer);
	std::string name = _shaderStruct.name + "Shader";

	std::vector<std::string> shaderNameParts;
	core::string::splitString(name, shaderNameParts, "_");
	std::string filename = "";
	for (std::string n : shaderNameParts) {
		if (n.length() > 1 || shaderNameParts.size() < 2) {
			n[0] = SDL_toupper(n[0]);
			filename += n;
		}
	}
	if (filename.empty()) {
		filename = name;
	}
	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", _namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", _shaderDirectory + _shaderStruct.filename);
	std::stringstream uniforms;
	std::stringstream uniformArrayInfo;
	const int uniformSize = int(_shaderStruct.uniforms.size());
	if (uniformSize > 0) {
		uniforms << "checkUniforms({";
		for (int i = 0; i < uniformSize; ++i) {
			std::string uniformName = _shaderStruct.uniforms[i].name;
			uniforms << "\"";
			uniforms << uniformName;
			uniforms << "\"";
			if (i < uniformSize - 1) {
				uniforms << ", ";
			}
		}
		uniforms << "});";

		for (int i = 0; i < uniformSize; ++i) {
			uniformArrayInfo << "\t\tsetUniformArraySize(\"";
			uniformArrayInfo << _shaderStruct.uniforms[i].name;
			uniformArrayInfo << "\", ";
			uniformArrayInfo << _shaderStruct.uniforms[i].arraySize;
			uniformArrayInfo << ");\n";
		}
	} else {
		uniforms << "// no uniforms";
	}
	src = core::string::replaceAll(src, "$uniformarrayinfo$", uniformArrayInfo.str());
	src = core::string::replaceAll(src, "$uniforms$", uniforms.str());

	std::stringstream attributes;
	const int attributeSize = int(_shaderStruct.attributes.size());
	if (attributeSize > 0) {
		attributes << "checkAttributes({";
		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = _shaderStruct.attributes[i];
			attributes << "\"" << v.name << "\"";
			if (i < attributeSize - 1) {
				attributes << ", ";
			}
		}
		attributes << "});\n";

		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = _shaderStruct.attributes[i];
			attributes << "\t\tconst int " << v.name << "Location = getAttributeLocation(\"" << v.name << "\");\n";
			attributes << "\t\tif (" << v.name << "Location != -1) {\n";
			attributes << "\t\t\tsetAttributeComponents(" << v.name << "Location, " << getComponents(v.type) << ");\n";
			attributes << "\t\t}\n";
		}
	} else {
		attributes << "// no attributes";
	}

	std::stringstream setters;
	std::stringstream includes;
	if (uniformSize > 0 || attributeSize > 0) {
		setters << "\n";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = _shaderStruct.uniforms[i];
		const bool isInteger = v.isSingleInteger();
		const std::string& uniformName = convertName(v.name, true);
		setters << "\tinline bool set" << uniformName << "(";
		const Types& cType = cTypes[v.type];
		if (v.arraySize > 0 && isInteger) {
			setters << "const ";
		} else if (cType.passBy == PassBy::Reference) {
			setters << "const ";
		}
		setters << cType.ctype;
		if (v.arraySize == -1 || cType.passBy == PassBy::Pointer) {
			setters << "*";
		} else if (cType.passBy == PassBy::Reference) {
			if (v.arraySize <= 0) {
				setters << "&";
			}
		} else if (cType.passBy == PassBy::Value) {
		}

		if (v.arraySize > 0) {
			setters << " (&" << v.name << ")[" << v.arraySize << "]";
		} else {
			setters << " " << v.name;
		}
		if (v.arraySize == -1) {
			setters << ", int amount";
		}
		setters << ") const {\n";

		setters << "\t\tconst int location = getUniformLocation(\"" << v.name;
		setters << "\");\n\t\tif (location == -1) {\n";
		setters << "\t\t\treturn false;\n";
		setters << "\t\t}\n";
		setters << "\t\tsetUniform" << uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
		setters << "(location, " << v.name;
		if (v.arraySize > 0) {
			setters << ", " << v.arraySize;
		} else if (v.arraySize == -1) {
			setters << ", amount";
		}
		setters << ");\n";
		setters << "\t\treturn true;\n";
		setters << "\t}\n";
		if (v.arraySize > 0) {
			setters << "\n\tinline bool set" << uniformName << "(" << "const std::vector<" << cType.ctype << ">& var) const {\n";
			setters << "\t\tconst int location = getUniformLocation(\"" << v.name;
			setters << "\");\n\t\tif (location == -1) {\n";
			setters << "\t\t\treturn false;\n";
			setters << "\t\t}\n";
			setters << "\t\tcore_assert((int)var.size() == " << v.arraySize << ");\n";
			setters << "\t\tsetUniform" << uniformSetterPostfix(v.type, v.arraySize) << "(location, &var.front(), var.size());\n";
			setters << "\t\treturn true;\n";
			setters << "\t}\n";
		} else if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4) {
			setters << "\n\tinline bool set" << uniformName << "(" << "const std::vector<float>& var) const {\n";
			setters << "\t\tconst int location = getUniformLocation(\"" << v.name;
			setters << "\");\n\t\tif (location == -1) {\n";
			setters << "\t\t\treturn false;\n";
			setters << "\t\t}\n";
			setters << "\t\tcore_assert(int(var.size()) % " << cType.components << " == 0);\n";
			setters << "\t\tsetUniformfv(location, &var.front(), " << cType.components << ", " << cType.components << ");\n";
			setters << "\t\treturn true;\n";
			setters << "\t}\n";
		}
		if (i < uniformSize- - 2) {
			setters << "\n";
		}

#if 0
		if (v.arraySize == -1 || v.arraySize > 1) {
			setters << "\tinline bool set" << uniformName << "(";
			const Types& cType = cTypes[v.type];
			setters << "const std::vector<" << cType.ctype << ">& " << v.name << ") const {\n";
			setters << "\t\tif (!hasUniform(\"" << v.name << "[0]\")) {\n";
			setters << "\t\t\treturn false;\n";
			setters << "\t\t}\n";
			setters << "\t\tsetUniform" << uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
			setters << "(\"" << v.name << "[0]\", &" << v.name << "[0], " << v.name << ".size());\n";
			setters << "\t\treturn true;\n";
			setters << "\t}\n";
			if (i < uniformSize- - 2) {
				setters << "\n";
			}
		}
#endif
	}
	for (int i = 0; i < attributeSize; ++i) {
		const Variable& v = _shaderStruct.attributes[i];
		const std::string& attributeName = convertName(v.name, true);
		const bool isInt = v.isInteger();
		setters << "\tinline bool init" << attributeName << "Custom(size_t stride = ";
		setters << "sizeof(" << cTypes[v.type].ctype << ")";
		setters << ", const void* pointer = nullptr, video::DataType type = ";
		if (isInt) {
			setters << "video::DataType::Int";
		} else {
			setters << "video::DataType::Float";
		}
		setters << ", int size = ";
		setters << cTypes[v.type].components << ", ";
		setters << "bool isInt = ";
		setters << (isInt ? "true" : "false");
		setters << ", bool normalize = false) const {\n";
		setters << "\t\tconst int loc = enableVertexAttributeArray(\"" << v.name << "\");\n";
		setters << "\t\tif (loc == -1) {\n";
		setters << "\t\t\treturn false;\n";
		setters << "\t\t}\n";
		setters << "\t\tif (isInt) {\n";
		setters << "\t\t\tsetVertexAttributeInt(loc, size, type, stride, pointer);\n";
		setters << "\t\t} else {\n";
		setters << "\t\t\tsetVertexAttribute(loc, size, type, normalize, stride, pointer);\n";
		setters << "\t\t}\n";
		setters << "\t\treturn true;\n";
		setters << "\t}\n\n";
		setters << "\tinline int getLocation" << attributeName << "() const {\n";
		setters << "\t\treturn getAttributeLocation(\"" << v.name << "\");\n";
		setters << "\t}\n\n";
		setters << "\tinline int getComponents" << attributeName << "() const {\n";
		setters << "\t\treturn getAttributeComponents(\"" << v.name << "\");\n";
		setters << "\t}\n\n";
		setters << "\tinline bool init" << attributeName << "() const {\n";
		setters << "\t\tconst int loc = enableVertexAttributeArray(\"" << v.name << "\");\n";
		setters << "\t\tif (loc == -1) {\n";
		setters << "\t\t\treturn false;\n";
		setters << "\t\t}\n";
		setters << "\t\tconst size_t stride = sizeof(" << cTypes[v.type].ctype << ");\n";
		setters << "\t\tconst void* pointer = nullptr;\n";
		setters << "\t\tconst video::DataType type = ";
		if (isInt) {
			setters << "video::DataType::Int";
		} else {
			setters << "video::DataType::Float";
		}
		setters << ";\n";
		setters << "\t\tconst int size = getAttributeComponents(loc);\n";
		if (isInt) {
			setters << "\t\tsetVertexAttributeInt(loc, size, type, stride, pointer);\n";
		} else {
			setters << "\t\tsetVertexAttribute(loc, size, type, false, stride, pointer);\n";
		}
		setters << "\t\treturn true;\n";
		setters << "\t}\n\n";
		setters << "\tinline bool set" << attributeName << "Divisor(uint32_t divisor) const {\n";
		setters << "\t\tconst int location = getAttributeLocation(\"" << v.name << "\");\n";
		setters << "\t\treturn setDivisor(location, divisor);\n";
		setters << "\t}\n";

		if (i < attributeSize - 1) {
			setters << "\n";
		}
	}

	std::stringstream ub;
	std::stringstream shutdown;
	if (!_shaderStruct.uniformBlocks.empty()) {
		setters << "\n";
	}
	for (auto & ubuf : _shaderStruct.uniformBlocks) {
		const std::string& uniformBufferStructName = convertName(ubuf.name, true);
		const std::string& uniformBufferName = convertName(ubuf.name, false);
		ub << "\n\t/**\n\t * @brief Uniform buffer for " << uniformBufferStructName << "::Data\n\t */\n";
		ub << "\tvideo::UniformBuffer _" << uniformBufferName << ";\n";
		shutdown << "\t\t_" << uniformBufferName << ".shutdown();\n";
		ub << "\t/**\n\t * @brief layout(std140) aligned uniform block structure\n\t */\n";
		ub << "\t#pragma pack(push, 1)\n\tstruct Data {\n";
		size_t structSize = 0u;
		int paddingCnt = 0;
		for (auto& v : ubuf.members) {
			const std::string& uniformName = convertName(v.name, false);
			const Types& cType = cTypes[v.type];
			ub << "\t\t" << std140Align(v) << cType.ctype << " " << uniformName;
			const size_t memberSize = std140Size(v);
			structSize += memberSize;
			if (v.arraySize > 0) {
				ub << "[" << v.arraySize << "]";
			}
			ub << "; // " << memberSize << " bytes\n";
			ub << std140Padding(v, paddingCnt);
		}
		ub << "\t};\n\t#pragma pack(pop)\n";
#if USE_ALIGN_AS > 0
		ub << "\tstatic_assert(sizeof(Data) == " << structSize << ", \"Unexpected structure size for Data\");\n";
#endif
		ub << "\n\tinline bool update(const Data& var) {\n";
		ub << "\t\treturn _" << uniformBufferName << ".update((const void*)&var, sizeof(var));\n";
		ub << "\t}\n\n";
		ub << "\n\tinline bool create(const Data& var) {\n";
		ub << "\t\treturn _" << uniformBufferName << ".create((const void*)&var, sizeof(var));\n";
		ub << "\t}\n\n";
		ub << "\n\tinline operator const video::UniformBuffer&() const {\n";
		ub << "\t\treturn _" << uniformBufferName << ";\n";
		ub << "\t}\n";
		setters << "\t/**\n";
		setters << "\t * @brief The the uniform buffer for the uniform block " << ubuf.name << "\n";
		setters << "\t */\n";
		setters << "\tinline bool set" << uniformBufferStructName << "(const video::UniformBuffer& buf) {\n";
		setters << "\t\treturn setUniformBuffer(\"" << ubuf.name << "\", buf);\n";
		setters << "\t}\n";

		std::string generatedUb = srcUb;
		generatedUb = core::string::replaceAll(generatedUb, "$name$", uniformBufferStructName);
		generatedUb = core::string::replaceAll(generatedUb, "$namespace$", _namespaceSrc);
		generatedUb = core::string::replaceAll(generatedUb, "$uniformbuffers$", ub.str());
		generatedUb = core::string::replaceAll(generatedUb, "$setters$", "");
		generatedUb = core::string::replaceAll(generatedUb, "$shutdown$", shutdown.str());

		const std::string targetFileUb = _sourceDirectory + uniformBufferStructName + ".h";

		includes << "#include \"" << uniformBufferStructName + ".h\"\n";

		Log::info("Generate ubo bindings for %s at %s", uniformBufferStructName.c_str(), targetFileUb.c_str());
		if (!core::App::getInstance()->filesystem()->syswrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			_exitCode = 100;
			requestQuit();
			return;
		}
	}

	src = core::string::replaceAll(src, "$attributes$", attributes.str());
	src = core::string::replaceAll(src, "$setters$", setters.str());
	src = core::string::replaceAll(src, "$includes$", includes.str());

	const std::string targetFile = _sourceDirectory + filename + ".h";
	Log::info("Generate shader bindings for %s at %s", _shaderStruct.name.c_str(), targetFile.c_str());
	if (!core::App::getInstance()->filesystem()->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		_exitCode = 100;
		requestQuit();
	}
}

bool ShaderTool::parseLayout(ShaderTool::Layout& layout, core::Tokenizer& tok) {
	if (!tok.hasNext()) {
		return false;
	}
	std::string token = tok.next();
	if (token != "(") {
		Log::warn("Unexpected layout syntax");
		return false;
	}
	do {
		if (!tok.hasNext()) {
			return false;
		}
		token = tok.next();
		if (token == "std140") {
			layout.blockLayout = BlockLayout::std140;
		} else if (token == "std430") {
			layout.blockLayout = BlockLayout::std430;
		}
		/* TODO location = x, offset = 20, std140
		int binding = 0;
		int components = 0;
		int offset = 0;
		int index = 0;
		*/
	} while(token != ")");

	return true;
}

bool ShaderTool::parse(const std::string& buffer, bool vertex) {
	bool uniformBlock = false;
	core::Tokenizer tok(buffer, " ;", "{}()");
	UniformBlock block;
	Layout layout;
	while (tok.hasNext()) {
		const std::string token = tok.next();
		Log::trace("token: %s", token.c_str());
		std::vector<Variable>* v = nullptr;
		if (token == "$in") {
			if (vertex) {
				v = &_shaderStruct.attributes;
			} else {
				// TODO: use this to validate that each $out of the vertex shader has a $in in the fragment shader
				//v = &_shaderStruct.varyings;
			}
		} else if (token == "$out") {
			if (vertex) {
				v = &_shaderStruct.varyings;
			} else {
				v = &_shaderStruct.outs;
			}
		} else if (token == "layout") {
			if (!parseLayout(layout, tok)) {
				Log::warn("Could not parse layout");
			}
		} else if (token == "buffer") {
			Log::warn("SSBO not supported");
		} else if (token == "uniform") {
			v = &_shaderStruct.uniforms;
		} else if (uniformBlock) {
			if (token == "}") {
				uniformBlock = false;
				Log::trace("End of uniform block: %s", block.name.c_str());
				_shaderStruct.uniformBlocks.push_back(block);
				layout = Layout();
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
		if (!tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get variable name for type %s", type.c_str());
			return false;
		}
		while (type == "highp" || type == "mediump" || type == "lowp" || type == "precision") {
			if (!tok.hasNext()) {
				Log::error("Failed to parse the shader, could not get type");
				return false;
			}
			type = tok.next();
		}
		std::string name = tok.next();
		// uniform block
		if (name == "{") {
			block.name = type;
			block.members.clear();
			Log::trace("Found uniform block: %s", type.c_str());
			uniformBlock = true;
			if (layout.blockLayout != BlockLayout::std140) {
				Log::warn("Incompatible block layout chosen for uniform block %s (%i)", type.c_str(), std::enum_value(layout.blockLayout));
			}
			continue;
		}
		const Variable::Type typeEnum = getType(type);
		bool isArray = false;
		std::string number;
		for (char c : name) {
			if (c == ']') {
				break;
			}
			if (isArray) {
				number.push_back(c);
			}
			if (c == '[') {
				isArray = true;
			}
		}
		int arraySize = core::string::toInt(number);
		if (isArray && arraySize == 0) {
			arraySize = -1;
			Log::warn("Could not determine array size for %s (%s)", name.c_str(), number.c_str());
		}
		if (isArray) {
			std::vector<std::string> tokens;
			core::string::splitString(name, tokens, "[");
			if (tokens.size() != 2) {
				Log::error("Could not extract variable name from %s", name.c_str());
			} else {
				name = tokens[0];
			}
		}
		// TODO: multi dimensional arrays are only supported in glsl >= 5.50
		if (uniformBlock) {
			block.members.push_back(Variable{typeEnum, name, arraySize});
		} else {
			auto findIter = std::find_if(v->begin(), v->end(), [&] (const Variable& var) { return var.name == name; });
			if (findIter == v->end()) {
				v->push_back(Variable{typeEnum, name, arraySize});
				layout = Layout();
			} else {
				Log::warn("Found duplicate variable %s (%s versus %s)",
						name.c_str(), cTypes[(int)findIter->type].ctype, cTypes[(int)typeEnum].ctype);
			}
		}
	}
	if (uniformBlock) {
		Log::error("Parsing error - still inside a uniform block");
		return false;
	}
	return true;
}

core::AppState ShaderTool::onConstruct() {
	registerArg("--glslang").setShort("-g").setDescription("Path to glslangvalidator binary").setMandatory();
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--buffertemplate").setShort("-b").setDescription("The uniform buffer template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("shader");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	Log::trace("Set some shader config vars to let the validation work");
	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
	return core::App::onConstruct();
}

core::AppState ShaderTool::onRunning() {
	const std::string glslangValidatorBin = getArgVal("--glslang");
	const std::string shaderfile          = getArgVal("--shader");
	_shaderTemplateFile                   = getArgVal("--shadertemplate");
	_uniformBufferTemplateFile            = getArgVal("--buffertemplate");
	_namespaceSrc                         = getArgVal("--namespace");
	_shaderDirectory                      = getArgVal("--shaderdir");
	_sourceDirectory                      = getArgVal("--sourcedir",
			_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");

	if (!core::string::endsWith(_shaderDirectory, "/")) {
		_shaderDirectory = _shaderDirectory + "/";
	}

	Log::debug("Using glslangvalidator binary: %s", glslangValidatorBin.c_str());
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_shaderfile = std::string(core::string::extractFilename(shaderfile.c_str()));
	Log::debug("Preparing shader file %s", _shaderfile.c_str());
	const std::string fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const bool changedDir = filesystem()->pushDir(std::string(core::string::extractPath(shaderfile.c_str())));
	const std::string fragmentBuffer = filesystem()->load(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const std::string vertexBuffer = filesystem()->load(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const std::string geometryBuffer = filesystem()->load(geometryFilename);

	video::Shader shader;

	const std::string& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, false);
	const std::string& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, false);

	_shaderStruct.filename = _shaderfile;
	_shaderStruct.name = _shaderfile;
	parse(fragmentSrcSource, false);
	if (!geometryBuffer.empty()) {
		const std::string& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, false);
		parse(geometrySrcSource, false);
	}
	parse(vertexSrcSource, true);
	generateSrc();

	const std::string& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, true);
	const std::string& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, true);
	const std::string& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, true);

	if (changedDir) {
		filesystem()->popDir();
	}

	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), filesystem()->homePath().c_str());
	std::string finalFragmentFilename = _appname + "-" + fragmentFilename;
	std::string finalVertexFilename = _appname + "-" + vertexFilename;
	std::string finalGeometryFilename = _appname + "-" + geometryFilename;
	filesystem()->write(finalFragmentFilename, fragmentSource);
	filesystem()->write(finalVertexFilename, vertexSource);
	if (!geometrySource.empty()) {
		filesystem()->write(finalGeometryFilename, geometrySource);
	}

	Log::debug("Validating shader file %s", _shaderfile.c_str());

	std::vector<std::string> fragmentArgs;
	fragmentArgs.push_back(filesystem()->homePath() + finalFragmentFilename);
	int fragmentValidationExitCode = core::Process::exec(glslangValidatorBin, fragmentArgs);

	std::vector<std::string> vertexArgs;
	vertexArgs.push_back(filesystem()->homePath() + finalVertexFilename);
	int vertexValidationExitCode = core::Process::exec(glslangValidatorBin, vertexArgs);

	int geometryValidationExitCode = 0;
	if (!geometrySource.empty()) {
		std::vector<std::string> geometryArgs;
		geometryArgs.push_back(filesystem()->homePath() + finalGeometryFilename);
		geometryValidationExitCode = core::Process::exec(glslangValidatorBin, geometryArgs);
	}

	if (fragmentValidationExitCode != 0) {
		Log::error("Failed to validate fragment shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), filesystem()->homePath().c_str(), finalFragmentFilename.c_str());
		_exitCode = fragmentValidationExitCode;
	} else if (vertexValidationExitCode != 0) {
		Log::error("Failed to validate vertex shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), filesystem()->homePath().c_str(), finalVertexFilename.c_str());
		_exitCode = vertexValidationExitCode;
	} else if (geometryValidationExitCode != 0) {
		Log::error("Failed to validate geometry shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), filesystem()->homePath().c_str(), finalGeometryFilename.c_str());
		_exitCode = geometryValidationExitCode;
	}

	requestQuit();
	return core::AppState::Running;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	ShaderTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
