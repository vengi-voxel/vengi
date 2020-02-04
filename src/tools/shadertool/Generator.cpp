/**
 * @file
 */

#include "Generator.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "video/Types.h"
#include "Util.h"
#include "core/String.h"
#include "core/StringUtil.h"

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

/**
 * @brief MSVC doesn't like strings that exceeds a certain length. So we have to split them up here.
 * @sa https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026?view=vs-2019
 */
static core::String maxStringLength(const core::String& input) {
#ifdef _MSC_VER
	if (input.size() > 10000) {
		Log::debug("Need to split the shader source string");
		return "R\"(" + core::string::replaceAll(input, "\n", ")\"\nR\"(") + ")\"";
	}
#endif
	return "R\"(" + input + ")\"";
}

bool generateSrc(const core::String& templateHeader, const core::String& templateSource, const core::String& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const core::String& namespaceSrc, const core::String& sourceDirectory, const core::String& shaderDirectory, const core::String& postfix,
		const core::String& vertexBuffer, const core::String& geometryBuffer, const core::String& fragmentBuffer, const core::String& computeBuffer) {
	core::String srcHeader(templateHeader);
	core::String srcSource(templateSource);
	const core::String& name = shaderStruct.name + "Shader";

	const core::String& filename = util::convertName(name, true);
	core::String uniforms;
	core::String uniformArrayInfo;
	const int uniformSize = int(shaderStruct.uniforms.size());
	if (uniformSize > 0) {
		uniforms += "checkUniforms({";
		for (int i = 0; i < uniformSize; ++i) {
			const core::String& uniformName = shaderStruct.uniforms[i].name;
			uniforms += "\"";
			uniforms += uniformName;
			uniforms += "\"";
			if (i < uniformSize - 1) {
				uniforms += ", ";
			}
		}
		uniforms += "});";

		for (int i = 0; i < uniformSize; ++i) {
			uniformArrayInfo += "\t\tsetUniformArraySize(\"";
			uniformArrayInfo += shaderStruct.uniforms[i].name;
			uniformArrayInfo += "\", ";
			uniformArrayInfo += core::string::toString(shaderStruct.uniforms[i].arraySize);
			uniformArrayInfo += ");\n";
		}
	} else {
		uniforms += "// no uniforms";
	}

	core::String attributes;
	const int attributeSize = int(shaderStruct.attributes.size());
	if (attributeSize > 0) {
		attributes += "checkAttributes({";
		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = shaderStruct.attributes[i];
			attributes += "\"";
			attributes += v.name;
			attributes += "\"";
			if (i < attributeSize - 1) {
				attributes += ", ";
			}
		}
		attributes += "});\n";

		for (int i = 0; i < attributeSize; ++i) {
			const Variable& v = shaderStruct.attributes[i];
			attributes += "\t\tconst int ";
			attributes += v.name;
			attributes += "Location = getAttributeLocation(\"";
			attributes += v.name;
			attributes += "\");\n";
			attributes += "\t\tif (";
			attributes += v.name;
			attributes += "Location != -1) {\n";
			attributes += "\t\t\tsetAttributeComponents(";
			attributes += v.name;
			attributes += "Location, ";
			attributes += core::string::toString(util::getComponents(v.type));
			attributes += ");\n";
			attributes += "\t\t}\n";
		}
	} else {
		attributes += "// no attributes";
	}

	core::String methods;
	core::String prototypes;
	if (uniformSize > 0 || attributeSize > 0) {
		methods += "\n";
		prototypes += "\n";
	}
	if (shaderStruct.out.layout.maxGeometryVertices > 0) {
		prototypes += "\n\tint getMaxGeometryVertices() const;\n";
		methods += "int ";
		methods += filename;
		methods += "::getMaxGeometryVertices() const {\n";
		methods += "\treturn ";
		methods += core::string::toString(shaderStruct.out.layout.maxGeometryVertices);
		methods += ";\n";
		methods += "}\n";
	}
	if (shaderStruct.out.layout.primitiveType != video::Primitive::Max) {
		prototypes += "\n\tvideo::Primitive getPrimitiveTypeOut() const;\n";
		methods += "\nvideo::Primitive ";
		methods += filename;
		methods += "::getPrimitiveTypeOut() const {\n";
		methods += "\treturn video::Primitive::";
		methods += util::getPrimitiveTypeString(shaderStruct.out.layout.primitiveType);
		methods += ";\n";
		methods += "}\n";
	}
	if (shaderStruct.in.layout.primitiveType != video::Primitive::Max) {
		prototypes += "\n\tvideo::Primitive getPrimitiveTypeIn() const;\n";
		methods += "\nvideo::Primitive ";
		methods += filename;
		methods += "::getPrimitiveTypeIn() const {\n";
		methods += "\treturn video::Primitive::";
		methods += util::getPrimitiveTypeString(shaderStruct.in.layout.primitiveType);
		methods += ";\n";
		methods += "}\n";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = shaderStruct.uniforms[i];
		const bool isInteger = v.isSingleInteger();
		const core::String& uniformName = util::convertName(v.name, true);
		core::String mproto;
		mproto += "set";
		mproto += uniformName;
		mproto += "(";
		const Types& cType = util::resolveTypes(v.type);
		auto layoutIter = shaderStruct.layouts.find(v.name);
		Layout layout;
		if (layoutIter != shaderStruct.layouts.end()) {
			layout = layoutIter->second;
		}

		if (v.arraySize > 0 && isInteger) {
			mproto += "const ";
		} else if (cType.passBy == PassBy::Reference) {
			mproto += "const ";
		}
		mproto += cType.ctype;
		if (v.arraySize == -1 || cType.passBy == PassBy::Pointer) {
			mproto += "*";
		} else if (cType.passBy == PassBy::Reference) {
			if (v.arraySize <= 0) {
				mproto += "&";
			}
		} else if (cType.passBy == PassBy::Value) {
		}

		if (v.arraySize > 0) {
			mproto += " (&";
			mproto += v.name;
			mproto += ")[";
			mproto += core::string::toString(v.arraySize);
			mproto += "]";
		} else {
			mproto += " ";
			mproto += v.name;
		}

		if (v.isSampler() && layout.binding != -1) {
			mproto += " = video::TextureUnit::";
			mproto += convertToTexUnit(layout.binding);
		}

		if (v.arraySize == -1) {
			mproto += ", int amount";
		}
		mproto += ") const";
		methods += "\nbool ";
		methods += filename;
		methods += "::";
		methods += mproto;
		methods += " {\n";
		prototypes += "\n";
		prototypes += "\t/**\n";
		prototypes += "\t * @brief Set the shader uniform value for ";
		prototypes += v.name;
		prototypes += "\n";
		prototypes += "\t * @note The uniform setter uses an internal cache and only perform the real update if something has changed.\n";
		prototypes += "\t */\n";
		prototypes += "\tbool ";
		prototypes += mproto;
		prototypes += ";\n";
		methods += "\tconst int location = ";
		if (layout.location != -1) {
			methods += core::string::toString(layout.location);
			methods += ";\n";
		} else {
			methods += "getUniformLocation(\"";
			methods += v.name;
			methods += "\");\n";
			methods += "\tif (location == -1) {\n";
			methods += "\t\treturn false;\n";
			methods += "\t}\n";
		}
		methods += "\tsetUniform";
		methods += util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
		methods += "(location, ";
		methods += v.name;
		if (v.arraySize > 0) {
			methods += ", ";
			methods += core::string::toString(v.arraySize);
		} else if (v.arraySize == -1) {
			methods += ", amount";
		}
		methods += ");\n";
		methods += "\treturn true;\n";
		methods += "}\n";

		if (v.isSampler()) {
			if (layout.binding != -1) {
				prototypes += "\n\tvideo::TextureUnit getBound";
				prototypes += uniformName;
				prototypes += "TexUnit() const;\n";
				methods += "\n\nvideo::TextureUnit ";
				methods += filename;
				methods += "::getBound";
				methods += uniformName;
				methods += "TexUnit() const {\n";
				methods += "\treturn video::TextureUnit::";
				methods += convertToTexUnit(layout.binding);
				methods += ";\n}\n";
			}
		}
		if (v.isSampler() || v.isImage()) {
			if (layout.imageFormat != video::ImageFormat::Max) {
				prototypes += "\n\tvideo::ImageFormat getImageFormat";
				prototypes += uniformName;
				prototypes += "() const;\n";
				methods += "\nvideo::ImageFormat ";
				methods += filename;
				methods += "::getImageFormat";
				methods += uniformName;
				methods += "() const {\n";
				methods += "\treturn video::ImageFormat::";
				methods += util::getImageFormatTypeString(layout.imageFormat);
				methods += ";\n}\n";
			}
		}
		if (layout.localSize.x != -1) {
			prototypes += "\n\tint getLocalSizeX() const;\n";
			methods += "\nint getLocalSizeX() const {\n";
			methods += "\treturn ";
			methods += core::string::toString(layout.localSize.x);
			methods += ";\n\t}\n";
		}
		if (layout.localSize.y != -1) {
			prototypes += "\n\tint getLocalSizeY() const;\n";
			methods += "\nint getLocalSizeY() const {\n";
			methods += "\treturn ";
			methods += core::string::toString(layout.localSize.y);
			methods += ";\n\t}\n";
		}
		if (layout.localSize.z != -1) {
			prototypes += "\n\tint getLocalSizeY() const;\n";
			methods += "\nint getLocalSizeZ() const {\n";
			methods += "\treturn ";
			methods += core::string::toString(layout.localSize.z);
			methods += ";\n\t}\n";
		}

		if (v.arraySize > 0) {
			prototypes += "\n\tbool set";
			prototypes += uniformName;
			prototypes += "(";
			prototypes += "const std::vector<";
			prototypes += cType.ctype;
			prototypes += ">& var) const;\n\n";
			methods += "\nbool ";
			methods += filename;
			methods += "::set";
			methods += uniformName;
			methods += "(";
			methods += "const std::vector<";
			methods += cType.ctype;
			methods += ">& var) const {\n";
			methods += "\tconst int location = getUniformLocation(\"";
			methods += v.name;
			methods += "\");\n\tif (location == -1) {\n";
			methods += "\t\treturn false;\n";
			methods += "\t}\n";
			methods += "\tcore_assert((int)var.size() == ";
			methods += core::string::toString(v.arraySize);
			methods += ");\n";
			methods += "\tsetUniform";
			methods += util::uniformSetterPostfix(v.type, v.arraySize);
			methods += "(location, &var.front(), var.size());\n";
			methods += "\treturn true;\n";
			methods += "}\n";
		} else if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4) {
			prototypes += "\n\tbool set";
			prototypes += uniformName;
			prototypes += "(";
			prototypes += "const std::vector<float>& var) const;\n";
			methods += "\nbool ";
			methods += filename;
			methods += "::set";
			methods += uniformName;
			methods += "(";
			methods += "const std::vector<float>& var) const {\n";
			methods += "\tconst int location = getUniformLocation(\"";
			methods += v.name;
			methods += "\");\n\tif (location == -1) {\n";
			methods += "\t\treturn false;\n";
			methods += "\t}\n";
			methods += "\tcore_assert(int(var.size()) % ";
			methods += core::string::toString(cType.components);
			methods += " == 0);\n";
			methods += "\tsetUniformfv(location, &var.front(), ";
			methods += core::string::toString(cType.components);
			methods += ", ";
			methods += core::string::toString(cType.components);
			methods += ");\n";
			methods += "\treturn true;\n";
			methods += "}\n";
		}
		if (i < uniformSize- - 2) {
			methods += "\n";
		}
	}
	for (int i = 0; i < attributeSize; ++i) {
		const Variable& v = shaderStruct.attributes[i];
		const core::String& attributeName = util::convertName(v.name, true);

		prototypes += "\n\t/**\n";
		prototypes += "\t * @brief This version takes the c++ data type as a reference\n";
		prototypes += "\t */\n";
		prototypes += "\ttemplate<typename CLASS, typename TYPE>\n";
		prototypes += "\tvideo::Attribute get";
		prototypes += attributeName;
		prototypes += "Attribute(int32_t bufferIndex, TYPE CLASS::* member, bool normalized = false) const {\n";
		prototypes += "\t\tvideo::Attribute attribute";
		prototypes += attributeName;
		prototypes += ";\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".bufferIndex = bufferIndex;\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".location = getLocation";
		prototypes += attributeName;
		prototypes += "();\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".size = getComponents";
		prototypes += attributeName;
		prototypes += "();\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".offset = reinterpret_cast<std::size_t>(&(((CLASS*)nullptr)->*member));\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".stride = sizeof(CLASS);\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".normalized = normalized;\n";
		prototypes += "\t\tattribute";
		prototypes += attributeName;
		prototypes += ".type = video::mapType<TYPE>();\n";
		// TODO: add validation that the given c++ data type fits the specified glsl type.
		prototypes += "\t\treturn attribute";
		prototypes += attributeName;
		prototypes += ";\n";
		prototypes += "\t}\n";

		prototypes += "\n\t/**\n\t * @brief Return the binding location of the shader attribute @c ";
		prototypes += attributeName;
		prototypes += "\n\t */\n";
		prototypes += "\tinline int getLocation";
		prototypes += attributeName;
		prototypes += "() const {\n";
		prototypes += "\t\treturn getAttributeLocation(\"";
		prototypes += v.name;
		prototypes += "\");\n";
		prototypes += "\t}\n";

		prototypes += "\n\t/**\n\t * @brief Return the components if the attribute @c ";
		prototypes += attributeName;
		prototypes += " is a vector type, or 1 if it is no vector\n\t */\n";
		prototypes += "\tstatic inline int getComponents";
		prototypes += attributeName;
		prototypes += "() {\n";
		prototypes += "\t\treturn ";
		prototypes += core::string::toString(util::getComponents(v.type));
		prototypes += ";\n";
		prototypes += "\t}\n";

		prototypes += "\n\t/**\n\t * @brief Used for instance rendering\n\t */\n";
		prototypes += "\tbool set";
		prototypes += attributeName;
		prototypes += "Divisor(uint32_t divisor) const;\n\n";
		methods += "\nbool ";
		methods += filename;
		methods += "::set";
		methods += attributeName;
		methods += "Divisor(uint32_t divisor) const {\n";
		methods += "\tconst int location = getAttributeLocation(\"";
		methods += v.name;
		methods += "\");\n";
		methods += "\treturn setDivisor(location, divisor);\n";
		methods += "}\n";

		if (i < attributeSize - 1) {
			methods += "\n";
		}
	}

	if (!shaderStruct.uniformBlocks.empty()) {
		methods += "\n";
	}
	core::String ub;
	core::String shutdown;
	core::String includes;
	const size_t uniformBlockAmount = shaderStruct.uniformBlocks.size();
	const core::String uniformBufferClassName = util::convertName(shaderStruct.name + "Data", true);
	for (auto & ubuf : shaderStruct.uniformBlocks) {
		const core::String& uniformBufferStructName = util::convertName(ubuf.name, true);
		const core::String& uniformBufferName = util::convertName(ubuf.name, false);
		ub += "\n\t/**\n\t * @brief Uniform buffer for ";
		ub += uniformBufferStructName;
		ub += "Data\n\t */\n";
		ub += "\tvideo::UniformBuffer _";
		ub += uniformBufferName;
		ub += ";\n";
		shutdown += "\t\t_";
		shutdown += uniformBufferName;
		shutdown += ".shutdown();\n";
		ub += "\t/**\n\t * @brief layout(";
		switch (ubuf.layout.blockLayout) {
		case BlockLayout::unknown:
		case BlockLayout::std140:
			ub += "std140";
			break;
		case BlockLayout::std430:
			ub += "std430";
			break;
		default:
			ub += "error";
			break;
		}
		ub += ") aligned uniform block structure\n\t */\n";
		ub += "\t#pragma pack(push, 1)\n\tstruct ";
		ub += uniformBufferStructName;
		ub += "Data {\n";
		size_t structSize = 0u;
		int paddingCnt = 0;
		for (auto& v : ubuf.members) {
			const core::String& uniformName = util::convertName(v.name, false);
			const Types& cType = util::resolveTypes(v.type);
			ub += "\t\t";
			ub += ubuf.layout.typeAlign(v);
			ub += cType.ctype;
			ub += " ";
			ub += uniformName;
			const size_t memberSize = ubuf.layout.typeSize(v);
			structSize += memberSize;
			if (v.arraySize > 0) {
				ub += "[";
				ub += core::string::toString(v.arraySize);
				ub += "]";
			}
			ub += "; // ";
			ub += core::string::toString(memberSize);
			ub += " bytes\n";
			ub += ubuf.layout.typePadding(v, paddingCnt);
		}
		ub += "\t};\n\t#pragma pack(pop)\n";
#if USE_ALIGN_AS > 0
		ub += "\tstatic_assert(sizeof(";
		ub += uniformBufferStructName;
		ub += "Data) == ";
		ub += core::string::toString(structSize);
		ub += ", \"Unexpected structure size for ";
		ub += uniformBufferStructName;
		ub += "Data\");\n";
#endif
		ub += "\n\tinline bool update(const ";
		ub += uniformBufferStructName;
		ub += "Data& var) {\n";
		ub += "\t\treturn _";
		ub += uniformBufferName;
		ub += ".update((const void*)&var, sizeof(var));\n";
		ub += "\t}\n\n";
		ub += "\n\tinline bool create(const ";
		ub += uniformBufferStructName;
		ub += "Data& var) {\n";
		ub += "\t\treturn _";
		ub += uniformBufferName;
		ub += ".create((const void*)&var, sizeof(var));\n";
		ub += "\t}\n\n";
		if (uniformBlockAmount == 1) {
			ub += "\n\tinline operator const video::UniformBuffer&() const {\n";
			ub += "\t\treturn _";
			ub += uniformBufferName;
			ub += ";\n";
			ub += "\t}\n\n";
		}
		ub += "\n\tinline const video::UniformBuffer& get";
		ub += uniformBufferStructName;
		ub += "UniformBuffer() const {\n";
		ub += "\t\treturn _";
		ub += uniformBufferName;
		ub += ";\n";
		ub += "\t}\n";
		prototypes += "\n\t/**\n";
		prototypes += "\t * @brief The the uniform buffer for the uniform block ";
		prototypes += ubuf.name;
		prototypes += "\n";
		prototypes += "\t */\n";
		prototypes += "\tinline bool set";
		prototypes += uniformBufferStructName;
		prototypes += "(const video::UniformBuffer& buf) {\n";
		prototypes += "\t\treturn setUniformBuffer(\"";
		prototypes += ubuf.name;
		prototypes += "\", buf);\n";
		prototypes += "\t}\n";

		core::String generatedUb = core::string::replaceAll(templateUniformBuffer, "$name$", uniformBufferClassName);
		generatedUb = core::string::replaceAll(generatedUb, "$namespace$", namespaceSrc);
		generatedUb = core::string::replaceAll(generatedUb, "$uniformbuffers$", ub);
		generatedUb = core::string::replaceAll(generatedUb, "$methods$", "");
		generatedUb = core::string::replaceAll(generatedUb, "$shutdown$", shutdown);
		const core::String targetFileUb = sourceDirectory + uniformBufferClassName + ".h";

		includes += "#include \"";
		includes += uniformBufferClassName;
		includes += ".h\"\n";

		Log::debug("Generate ubo bindings for %s at %s", uniformBufferStructName.c_str(), targetFileUb.c_str());
		if (!filesystem->syswrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			return false;
		}
	}

	for (const auto& e : shaderStruct.constants) {
		prototypes += "\t/**\n";
		prototypes += "\t * @brief Exported from shader code by @code $constant ";
		prototypes += e.first;
		prototypes += " ";
		prototypes += e.second;
		prototypes += " @endcode\n";
		prototypes += "\t */\n";
		if (core::string::isInteger(e.second)) {
			prototypes += "\tinline static constexpr int get";
			prototypes += util::convertName(e.first, true);
			prototypes += "() {\n";
			prototypes += "\t\treturn ";
			prototypes += e.second;
			prototypes += ";\n";
			prototypes += "\t}\n";
		} else if (core::string::isNumber(e.second)) {
			prototypes += "\tinline static constexpr double get";
			prototypes += util::convertName(e.first, true);
			prototypes += "() {\n";
			prototypes += "\t\treturn ";
			prototypes += e.second;
			prototypes += ";\n";
			prototypes += "\t}\n";
		} else {
			prototypes += "\tinline static constexpr const char* get";
			prototypes += util::convertName(e.first, true);
			prototypes += "() {\n";
			prototypes += "\t\treturn \"";
			prototypes += e.second;
			prototypes += "\";\n";
			prototypes += "\t}\n";
		}
	}

	srcHeader = core::string::replaceAll(srcHeader, "$name$", filename);
	srcHeader = core::string::replaceAll(srcHeader, "$namespace$", namespaceSrc);
	srcHeader = core::string::replaceAll(srcHeader, "$filename$", shaderDirectory + shaderStruct.filename);
	srcHeader = core::string::replaceAll(srcHeader, "$uniformarrayinfo$", uniformArrayInfo);
	srcHeader = core::string::replaceAll(srcHeader, "$uniforms$", uniforms);

	srcHeader = core::string::replaceAll(srcHeader, "$attributes$", attributes);
	srcHeader = core::string::replaceAll(srcHeader, "$methods$", methods);
	srcHeader = core::string::replaceAll(srcHeader, "$prototypes$", prototypes);
	srcHeader = core::string::replaceAll(srcHeader, "$includes$", includes);

	srcHeader = core::string::replaceAll(srcHeader, "$vertexshaderbuffer$", vertexBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$computeshaderbuffer$", computeBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$fragmentshaderbuffer$", fragmentBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$geometryshaderbuffer$", geometryBuffer);

	srcSource = core::string::replaceAll(srcSource, "$name$", filename);
	srcSource = core::string::replaceAll(srcSource, "$namespace$", namespaceSrc);
	srcSource = core::string::replaceAll(srcSource, "$filename$", shaderDirectory + shaderStruct.filename);
	srcSource = core::string::replaceAll(srcSource, "$uniformarrayinfo$", uniformArrayInfo);
	srcSource = core::string::replaceAll(srcSource, "$uniforms$", uniforms);

	srcSource = core::string::replaceAll(srcSource, "$attributes$", attributes);
	srcSource = core::string::replaceAll(srcSource, "$methods$", methods);
	srcSource = core::string::replaceAll(srcSource, "$prototypes$", prototypes);
	srcSource = core::string::replaceAll(srcSource, "$includes$", includes);

	srcSource = core::string::replaceAll(srcSource, "$vertexshaderbuffer$", maxStringLength(vertexBuffer));
	srcSource = core::string::replaceAll(srcSource, "$computeshaderbuffer$", maxStringLength(computeBuffer));
	srcSource = core::string::replaceAll(srcSource, "$fragmentshaderbuffer$", maxStringLength(fragmentBuffer));
	srcSource = core::string::replaceAll(srcSource, "$geometryshaderbuffer$", maxStringLength(geometryBuffer));

	Log::debug("Generate shader bindings for %s", shaderStruct.name.c_str());
	const core::String targetHeaderFile = sourceDirectory + filename + ".h" + postfix;
	if (!filesystem->syswrite(targetHeaderFile, srcHeader)) {
		Log::error("Failed to write %s", targetHeaderFile.c_str());
		return false;
	}

	const core::String targetSourceFile = sourceDirectory + filename + ".cpp" + postfix;
	if (!filesystem->syswrite(targetSourceFile, srcSource)) {
		Log::error("Failed to write %s", targetSourceFile.c_str());
		return false;
	}
	return true;
}

}
