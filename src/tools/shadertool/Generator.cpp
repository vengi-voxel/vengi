/**
 * @file
 */

#include "Generator.h"
#include "core/Log.h"
#include "core/String.h"
#include "video/Types.h"
#include "Util.h"
#include <string>
#include <sstream>

namespace shadertool {

static const char *convertToTexUnit(int unit) {
	switch (unit) {
	default:
		// fallthrough
	case 0:
		return "Zero";
	case 1:
		return "One";
	case 2:
		return "Two";
	case 3:
		return "Three";
	case 4:
		return "Four";
	case 5:
		return "Five";
	}
}

bool generateSrc(const std::string& templateShader, const std::string& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const std::string& namespaceSrc, const std::string& sourceDirectory, const std::string& shaderDirectory, const std::string& postfix) {
	std::string src(templateShader);
	const std::string& name = shaderStruct.name + "Shader";

	const std::string& filename = util::convertName(name, true);
	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", shaderDirectory + shaderStruct.filename);
	std::stringstream uniforms;
	std::stringstream uniformArrayInfo;
	const int uniformSize = int(shaderStruct.uniforms.size());
	if (uniformSize > 0) {
		uniforms << "checkUniforms({";
		for (int i = 0; i < uniformSize; ++i) {
			const std::string& uniformName = shaderStruct.uniforms[i].name;
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
			uniformArrayInfo << shaderStruct.uniforms[i].name;
			uniformArrayInfo << "\", ";
			uniformArrayInfo << shaderStruct.uniforms[i].arraySize;
			uniformArrayInfo << ");\n";
		}
	} else {
		uniforms << "// no uniforms";
	}
	src = core::string::replaceAll(src, "$uniformarrayinfo$", uniformArrayInfo.str());
	src = core::string::replaceAll(src, "$uniforms$", uniforms.str());

	std::stringstream attributes;
	const int attributeSize = int(shaderStruct.attributes.size());
	if (attributeSize > 0) {
		attributes << "checkAttributes({";
		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = shaderStruct.attributes[i];
			attributes << "\"" << v.name << "\"";
			if (i < attributeSize - 1) {
				attributes << ", ";
			}
		}
		attributes << "});\n";

		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = shaderStruct.attributes[i];
			attributes << "\t\tconst int " << v.name << "Location = getAttributeLocation(\"" << v.name << "\");\n";
			attributes << "\t\tif (" << v.name << "Location != -1) {\n";
			attributes << "\t\t\tsetAttributeComponents(" << v.name << "Location, " << util::getComponents(v.type) << ");\n";
			attributes << "\t\t}\n";
		}
	} else {
		attributes << "// no attributes";
	}

	std::stringstream methods;
	if (uniformSize > 0 || attributeSize > 0) {
		methods << "\n";
	}
	if (shaderStruct.out.layout.maxGeometryVertices > 0) {
		methods << "\tint getMaxGeometryVertices() const {\n";
		methods << "\t\treturn " << shaderStruct.out.layout.maxGeometryVertices << ";\n";
		methods << "\t}\n\n;";
	}
	if (shaderStruct.out.layout.primitiveType != video::Primitive::Max) {
		methods << "\tvideo::Primitive getPrimitiveTypeOut() const {\n";
		methods << "\t\treturn video::Primitive::" << util::getPrimitiveTypeString(shaderStruct.out.layout.primitiveType) << ";\n";
		methods << "\t}\n\n;";
	}
	if (shaderStruct.in.layout.primitiveType != video::Primitive::Max) {
		methods << "\tvideo::Primitive getPrimitiveTypeIn() const {\n";
		methods << "\t\treturn video::Primitive::" << util::getPrimitiveTypeString(shaderStruct.in.layout.primitiveType) << ";\n";
		methods << "\t}\n\n;";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = shaderStruct.uniforms[i];
		const bool isInteger = v.isSingleInteger();
		const std::string& uniformName = util::convertName(v.name, true);
		methods << "\tinline bool set" << uniformName << "(";
		const Types& cType = util::resolveTypes(v.type);
		auto layoutIter = shaderStruct.layouts.find(v.name);
		Layout layout;
		if (layoutIter != shaderStruct.layouts.end()) {
			layout = layoutIter->second;
		}

		if (v.arraySize > 0 && isInteger) {
			methods << "const ";
		} else if (cType.passBy == PassBy::Reference) {
			methods << "const ";
		}
		methods << cType.ctype;
		if (v.arraySize == -1 || cType.passBy == PassBy::Pointer) {
			methods << "*";
		} else if (cType.passBy == PassBy::Reference) {
			if (v.arraySize <= 0) {
				methods << "&";
			}
		} else if (cType.passBy == PassBy::Value) {
		}

		if (v.arraySize > 0) {
			methods << " (&" << v.name << ")[" << v.arraySize << "]";
		} else {
			methods << " " << v.name;
		}

		if (v.isSampler() && layout.binding != -1) {
			methods << " = video::TextureUnit::" << convertToTexUnit(layout.binding);
		}

		if (v.arraySize == -1) {
			methods << ", int amount";
		}
		methods << ") const {\n";

		methods << "\t\tconst int location = ";
		if (layout.location != -1) {
			methods << layout.location << ";\n";
		} else {
			methods << "getUniformLocation(\"" << v.name << "\");\n";
			methods << "\t\tif (location == -1) {\n";
			methods << "\t\t\treturn false;\n";
			methods << "\t\t}\n";
		}
		methods << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
		methods << "(location, " << v.name;
		if (v.arraySize > 0) {
			methods << ", " << v.arraySize;
		} else if (v.arraySize == -1) {
			methods << ", amount";
		}
		methods << ");\n";
		methods << "\t\treturn true;\n";
		methods << "\t}\n";

		if (v.isSampler()) {
			if (layout.binding != -1) {
				methods << "\n\tinline video::TextureUnit getBound" << uniformName << "TexUnit() const {\n";
				methods << "\t\treturn video::TextureUnit::" << convertToTexUnit(layout.binding) << ";\n\t}\n";
			}
		}
		if (v.isSampler() || v.isImage()) {
			if (layout.imageFormat != video::ImageFormat::Max) {
				methods << "\n\tinline video::ImageFormat getImageFormat" << uniformName << "() const {\n";
				methods << "\t\treturn video::ImageFormat::" << util::getImageFormatTypeString(layout.imageFormat) << ";\n\t}\n";
			}
		}
		if (layout.localSize.x != -1) {
			methods << "\n\tinline int getLocalSizeX() const {\n";
			methods << "\t\treturn " << layout.localSize.x << ";\n\t}\n";
		}
		if (layout.localSize.y != -1) {
			methods << "\n\tinline int getLocalSizeY() const {\n";
			methods << "\t\treturn " << layout.localSize.y << ";\n\t}\n";
		}
		if (layout.localSize.z != -1) {
			methods << "\n\tinline int getLocalSizeZ() const {\n";
			methods << "\t\treturn " << layout.localSize.z << ";\n\t}\n";
		}

		if (v.arraySize > 0) {
			methods << "\n\tinline bool set" << uniformName << "(" << "const std::vector<" << cType.ctype << ">& var) const {\n";
			methods << "\t\tconst int location = getUniformLocation(\"" << v.name;
			methods << "\");\n\t\tif (location == -1) {\n";
			methods << "\t\t\treturn false;\n";
			methods << "\t\t}\n";
			methods << "\t\tcore_assert((int)var.size() == " << v.arraySize << ");\n";
			methods << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize) << "(location, &var.front(), var.size());\n";
			methods << "\t\treturn true;\n";
			methods << "\t}\n";
		} else if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4) {
			methods << "\n\tinline bool set" << uniformName << "(" << "const std::vector<float>& var) const {\n";
			methods << "\t\tconst int location = getUniformLocation(\"" << v.name;
			methods << "\");\n\t\tif (location == -1) {\n";
			methods << "\t\t\treturn false;\n";
			methods << "\t\t}\n";
			methods << "\t\tcore_assert(int(var.size()) % " << cType.components << " == 0);\n";
			methods << "\t\tsetUniformfv(location, &var.front(), " << cType.components << ", " << cType.components << ");\n";
			methods << "\t\treturn true;\n";
			methods << "\t}\n";
		}
		if (i < uniformSize- - 2) {
			methods << "\n";
		}

#if 0
		if (v.arraySize == -1 || v.arraySize > 1) {
			methods << "\tinline bool set" << uniformName << "(";
			const Types& cType = util::getTypes(v.type);
			methods << "const std::vector<" << cType.ctype << ">& " << v.name << ") const {\n";
			methods << "\t\tif (!hasUniform(\"" << v.name << "[0]\")) {\n";
			methods << "\t\t\treturn false;\n";
			methods << "\t\t}\n";
			methods << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
			methods << "(\"" << v.name << "[0]\", &" << v.name << "[0], " << v.name << ".size());\n";
			methods << "\t\treturn true;\n";
			methods << "\t}\n";
			if (i < uniformSize- - 2) {
				methods << "\n";
			}
		}
#endif
	}
	for (int i = 0; i < attributeSize; ++i) {
		const Variable& v = shaderStruct.attributes[i];
		const std::string& attributeName = util::convertName(v.name, true);
		const bool isInt = v.isInteger();

		methods << "\tvideo::Attribute get" << attributeName << "Attribute(int32_t bufferIndex, int stride = 0, intptr_t offset = 0, bool normalized = false) const {\n";
		methods << "\t\tvideo::Attribute attribute" << attributeName << ";\n";
		methods << "\t\tattribute" << attributeName << ".bufferIndex = bufferIndex;\n";
		methods << "\t\tattribute" << attributeName << ".index = getLocation" << attributeName << "();\n";
		methods << "\t\tattribute" << attributeName << ".size = getComponents" << attributeName << "();\n";
		methods << "\t\tattribute" << attributeName << ".offset = offset;\n";
		methods << "\t\tattribute" << attributeName << ".stride = stride;\n";
		methods << "\t\tattribute" << attributeName << ".normalized = normalized;\n";
		methods << "\t\tattribute" << attributeName << ".type = ";
		if (isInt) {
			methods << "video::DataType::Int;\n";
		} else {
			methods << "video::DataType::Float;\n";
		}
		methods << "\t\treturn attribute" << attributeName << ";\n";
		methods << "\t};\n\n";

		methods << "\ttemplate<typename CLASS, typename TYPE>\n";
		methods << "\tvideo::Attribute get" << attributeName << "Attribute(int32_t bufferIndex, TYPE CLASS::* member, bool normalized = false) const {\n";
		methods << "\t\tvideo::Attribute attribute" << attributeName << ";\n";
		methods << "\t\tattribute" << attributeName << ".bufferIndex = bufferIndex;\n";
		methods << "\t\tattribute" << attributeName << ".index = getLocation" << attributeName << "();\n";
		methods << "\t\tattribute" << attributeName << ".size = getComponents" << attributeName << "();\n";
		methods << "\t\tattribute" << attributeName << ".offset = reinterpret_cast<std::size_t>(&(((CLASS*)nullptr)->*member));\n";
		methods << "\t\tattribute" << attributeName << ".stride = sizeof(CLASS);\n";
		methods << "\t\tattribute" << attributeName << ".normalized = normalized;\n";
		methods << "\t\tattribute" << attributeName << ".type = video::mapType<TYPE>();\n";
		methods << "\t\treturn attribute" << attributeName << ";\n";
		methods << "\t};\n\n";

		methods << "\tinline bool init" << attributeName << "Custom(size_t stride = ";
		methods << "sizeof(" << util::resolveTypes(v.type).ctype << ")";
		methods << ", const void* pointer = nullptr, video::DataType type = ";
		if (isInt) {
			methods << "video::DataType::Int";
		} else {
			methods << "video::DataType::Float";
		}
		methods << ", int size = ";
		methods << util::resolveTypes(v.type).components << ", ";
		methods << "bool isInt = ";
		methods << (isInt ? "true" : "false");
		methods << ", bool normalize = false) const {\n";
		methods << "\t\tconst int loc = enableVertexAttributeArray(\"" << v.name << "\");\n";
		methods << "\t\tif (loc == -1) {\n";
		methods << "\t\t\treturn false;\n";
		methods << "\t\t}\n";
		methods << "\t\tif (isInt) {\n";
		methods << "\t\t\tsetVertexAttributeInt(loc, size, type, stride, pointer);\n";
		methods << "\t\t} else {\n";
		methods << "\t\t\tsetVertexAttribute(loc, size, type, normalize, stride, pointer);\n";
		methods << "\t\t}\n";
		methods << "\t\treturn true;\n";
		methods << "\t}\n\n";
		methods << "\tinline int getLocation" << attributeName << "() const {\n";
		methods << "\t\treturn getAttributeLocation(\"" << v.name << "\");\n";
		methods << "\t}\n\n";
		methods << "\tinline int getComponents" << attributeName << "() const {\n";
		methods << "\t\treturn getAttributeComponents(\"" << v.name << "\");\n";
		methods << "\t}\n\n";
		methods << "\tinline bool init" << attributeName << "() const {\n";
		methods << "\t\tconst int loc = enableVertexAttributeArray(\"" << v.name << "\");\n";
		methods << "\t\tif (loc == -1) {\n";
		methods << "\t\t\treturn false;\n";
		methods << "\t\t}\n";
		methods << "\t\tconst size_t stride = sizeof(" << util::resolveTypes(v.type).ctype << ");\n";
		methods << "\t\tconst void* pointer = nullptr;\n";
		methods << "\t\tconst video::DataType type = ";
		if (isInt) {
			methods << "video::DataType::Int";
		} else {
			methods << "video::DataType::Float";
		}
		methods << ";\n";
		methods << "\t\tconst int size = getAttributeComponents(loc);\n";
		if (isInt) {
			methods << "\t\tsetVertexAttributeInt(loc, size, type, stride, pointer);\n";
		} else {
			methods << "\t\tsetVertexAttribute(loc, size, type, false, stride, pointer);\n";
		}
		methods << "\t\treturn true;\n";
		methods << "\t}\n\n";
		methods << "\tinline bool set" << attributeName << "Divisor(uint32_t divisor) const {\n";
		methods << "\t\tconst int location = getAttributeLocation(\"" << v.name << "\");\n";
		methods << "\t\treturn setDivisor(location, divisor);\n";
		methods << "\t}\n";

		if (i < attributeSize - 1) {
			methods << "\n";
		}
	}

	if (!shaderStruct.uniformBlocks.empty()) {
		methods << "\n";
	}
	std::stringstream ub;
	std::stringstream shutdown;
	std::stringstream includes;
	for (auto & ubuf : shaderStruct.uniformBlocks) {
		const std::string& uniformBufferStructName = util::convertName(ubuf.name, true);
		const std::string& uniformBufferName = util::convertName(ubuf.name, false);
		ub << "\n\t/**\n\t * @brief Uniform buffer for " << uniformBufferStructName << "::Data\n\t */\n";
		ub << "\tvideo::UniformBuffer _" << uniformBufferName << ";\n";
		shutdown << "\t\t_" << uniformBufferName << ".shutdown();\n";
		ub << "\t/**\n\t * @brief layout(";
		switch (ubuf.layout.blockLayout) {
		case BlockLayout::unknown:
		case BlockLayout::std140:
			ub << "std140";
			break;
		case BlockLayout::std430:
			ub << "std430";
			break;
		default:
			ub << "error";
			break;
		}
		ub << ") aligned uniform block structure\n\t */\n";
		ub << "\t#pragma pack(push, 1)\n\tstruct Data {\n";
		size_t structSize = 0u;
		int paddingCnt = 0;
		for (auto& v : ubuf.members) {
			const std::string& uniformName = util::convertName(v.name, false);
			const Types& cType = util::resolveTypes(v.type);
			ub << "\t\t" << ubuf.layout.typeAlign(v) << cType.ctype << " " << uniformName;
			const size_t memberSize = ubuf.layout.typeSize(v);
			structSize += memberSize;
			if (v.arraySize > 0) {
				ub << "[" << v.arraySize << "]";
			}
			ub << "; // " << memberSize << " bytes\n";
			ub << ubuf.layout.typePadding(v, paddingCnt);
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
		methods << "\t/**\n";
		methods << "\t * @brief The the uniform buffer for the uniform block " << ubuf.name << "\n";
		methods << "\t */\n";
		methods << "\tinline bool set" << uniformBufferStructName << "(const video::UniformBuffer& buf) {\n";
		methods << "\t\treturn setUniformBuffer(\"" << ubuf.name << "\", buf);\n";
		methods << "\t}\n";

		std::string generatedUb = core::string::replaceAll(templateUniformBuffer, "$name$", uniformBufferStructName);
		generatedUb = core::string::replaceAll(generatedUb, "$namespace$", namespaceSrc);
		generatedUb = core::string::replaceAll(generatedUb, "$uniformbuffers$", ub.str());
		generatedUb = core::string::replaceAll(generatedUb, "$methods$", "");
		generatedUb = core::string::replaceAll(generatedUb, "$shutdown$", shutdown.str());

		const std::string targetFileUb = sourceDirectory + uniformBufferStructName + ".h";

		includes << "#include \"" << uniformBufferStructName + ".h\"\n";

		Log::debug("Generate ubo bindings for %s at %s", uniformBufferStructName.c_str(), targetFileUb.c_str());
		if (!filesystem->syswrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			return false;
		}
	}

	for (const auto& e : shaderStruct.constants) {
		methods << "\t/**\n";
		methods << "\t * @brief Exported from shader code by @code $constant " << e.first << " " << e.second << " @endcode\n";
		methods << "\t */\n";
		methods << "\tinline static const char* get" << util::convertName(e.first, true) << "() {\n";
		methods << "\t\treturn \"" << e.second << "\";\n";
		methods << "\t}\n";
	}

	src = core::string::replaceAll(src, "$attributes$", attributes.str());
	src = core::string::replaceAll(src, "$methods$", methods.str());
	src = core::string::replaceAll(src, "$includes$", includes.str());

	const std::string targetFile = sourceDirectory + filename + ".h" + postfix;
	Log::debug("Generate shader bindings for %s at %s", shaderStruct.name.c_str(), targetFile.c_str());
	if (!filesystem->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		return false;
	}
	return true;
}

}
