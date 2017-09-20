/**
 * @file
 */

#include "Generator.h"
#include "core/Log.h"
#include "core/String.h"
#include "Util.h"
#include <string>
#include <sstream>

namespace shadertool {

bool generateSrc(const std::string& templateShader, const std::string& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const std::string& namespaceSrc, const std::string& sourceDirectory, const std::string& shaderDirectory) {
	std::string src(templateShader);
	const std::string& name = shaderStruct.name + "Shader";

	const std::string& filename = util::convertName(name, false);
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

	std::stringstream setters;
	std::stringstream includes;
	if (uniformSize > 0 || attributeSize > 0) {
		setters << "\n";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = shaderStruct.uniforms[i];
		const bool isInteger = v.isSingleInteger();
		const std::string& uniformName = util::convertName(v.name, true);
		setters << "\tinline bool set" << uniformName << "(";
		const Types& cType = util::resolveTypes(v.type);
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
		setters << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
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
			setters << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize) << "(location, &var.front(), var.size());\n";
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
			const Types& cType = util::getTypes(v.type);
			setters << "const std::vector<" << cType.ctype << ">& " << v.name << ") const {\n";
			setters << "\t\tif (!hasUniform(\"" << v.name << "[0]\")) {\n";
			setters << "\t\t\treturn false;\n";
			setters << "\t\t}\n";
			setters << "\t\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
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
		const Variable& v = shaderStruct.attributes[i];
		const std::string& attributeName = util::convertName(v.name, true);
		const bool isInt = v.isInteger();
		setters << "\tinline bool init" << attributeName << "Custom(size_t stride = ";
		setters << "sizeof(" << util::resolveTypes(v.type).ctype << ")";
		setters << ", const void* pointer = nullptr, video::DataType type = ";
		if (isInt) {
			setters << "video::DataType::Int";
		} else {
			setters << "video::DataType::Float";
		}
		setters << ", int size = ";
		setters << util::resolveTypes(v.type).components << ", ";
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
		setters << "\t\tconst size_t stride = sizeof(" << util::resolveTypes(v.type).ctype << ");\n";
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
	if (!shaderStruct.uniformBlocks.empty()) {
		setters << "\n";
	}
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
		setters << "\t/**\n";
		setters << "\t * @brief The the uniform buffer for the uniform block " << ubuf.name << "\n";
		setters << "\t */\n";
		setters << "\tinline bool set" << uniformBufferStructName << "(const video::UniformBuffer& buf) {\n";
		setters << "\t\treturn setUniformBuffer(\"" << ubuf.name << "\", buf);\n";
		setters << "\t}\n";

		std::string generatedUb = core::string::replaceAll(templateUniformBuffer, "$name$", uniformBufferStructName);
		generatedUb = core::string::replaceAll(generatedUb, "$namespace$", namespaceSrc);
		generatedUb = core::string::replaceAll(generatedUb, "$uniformbuffers$", ub.str());
		generatedUb = core::string::replaceAll(generatedUb, "$setters$", "");
		generatedUb = core::string::replaceAll(generatedUb, "$shutdown$", shutdown.str());

		const std::string targetFileUb = sourceDirectory + uniformBufferStructName + ".h";

		includes << "#include \"" << uniformBufferStructName + ".h\"\n";

		Log::info("Generate ubo bindings for %s at %s", uniformBufferStructName.c_str(), targetFileUb.c_str());
		if (!filesystem->syswrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			return false;
		}
	}

	src = core::string::replaceAll(src, "$attributes$", attributes.str());
	src = core::string::replaceAll(src, "$setters$", setters.str());
	src = core::string::replaceAll(src, "$includes$", includes.str());

	const std::string targetFile = sourceDirectory + filename + ".h";
	Log::info("Generate shader bindings for %s at %s", shaderStruct.name.c_str(), targetFile.c_str());
	if (!filesystem->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		return false;
	}
	return true;
}

}
