/**
 * @file
 */

#include "Generator.h"
#include "core/Log.h"
#include "core/StringUtil.h"
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

/**
 * @brief MSVC doesn't like strings that exceeds a certain length. So we have to split them up here.
 * @sa https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026?view=vs-2019
 */
static std::string maxStringLength(const std::string& input) {
#ifdef _MSC_VER
	if (input.length() > 10000) {
		Log::debug("Need to split the shader source string");
		return "R\"(" + core::string::replaceAll(input, "\n", ")\"\nR\"(") + ")\"";
	}
#endif
	return "R\"(" + input + ")\"";
}

bool generateSrc(const std::string& templateHeader, const std::string& templateSource, const std::string& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const std::string& namespaceSrc, const std::string& sourceDirectory, const std::string& shaderDirectory, const std::string& postfix,
		const std::string& vertexBuffer, const std::string& geometryBuffer, const std::string& fragmentBuffer, const std::string& computeBuffer) {
	std::string srcHeader(templateHeader);
	std::string srcSource(templateSource);
	const std::string& name = shaderStruct.name + "Shader";

	const std::string& filename = util::convertName(name, true);
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
	std::stringstream prototypes;
	if (uniformSize > 0 || attributeSize > 0) {
		methods << "\n";
		prototypes << "\n";
	}
	if (shaderStruct.out.layout.maxGeometryVertices > 0) {
		prototypes << "\n\tint getMaxGeometryVertices() const;\n";
		methods << "int " << filename << "::getMaxGeometryVertices() const {\n";
		methods << "\treturn " << shaderStruct.out.layout.maxGeometryVertices << ";\n";
		methods << "}\n";
	}
	if (shaderStruct.out.layout.primitiveType != video::Primitive::Max) {
		prototypes << "\n\tvideo::Primitive getPrimitiveTypeOut() const;\n";
		methods << "\nvideo::Primitive " << filename << "::getPrimitiveTypeOut() const {\n";
		methods << "\treturn video::Primitive::" << util::getPrimitiveTypeString(shaderStruct.out.layout.primitiveType) << ";\n";
		methods << "}\n";
	}
	if (shaderStruct.in.layout.primitiveType != video::Primitive::Max) {
		prototypes << "\n\tvideo::Primitive getPrimitiveTypeIn() const;\n";
		methods << "\nvideo::Primitive " << filename << "::getPrimitiveTypeIn() const {\n";
		methods << "\treturn video::Primitive::" << util::getPrimitiveTypeString(shaderStruct.in.layout.primitiveType) << ";\n";
		methods << "}\n";
	}
	for (int i = 0; i < uniformSize; ++i) {
		const Variable& v = shaderStruct.uniforms[i];
		const bool isInteger = v.isSingleInteger();
		const std::string& uniformName = util::convertName(v.name, true);
		std::stringstream mproto;
		mproto << "set" << uniformName << "(";
		const Types& cType = util::resolveTypes(v.type);
		auto layoutIter = shaderStruct.layouts.find(v.name);
		Layout layout;
		if (layoutIter != shaderStruct.layouts.end()) {
			layout = layoutIter->second;
		}

		if (v.arraySize > 0 && isInteger) {
			mproto << "const ";
		} else if (cType.passBy == PassBy::Reference) {
			mproto << "const ";
		}
		mproto << cType.ctype;
		if (v.arraySize == -1 || cType.passBy == PassBy::Pointer) {
			mproto << "*";
		} else if (cType.passBy == PassBy::Reference) {
			if (v.arraySize <= 0) {
				mproto << "&";
			}
		} else if (cType.passBy == PassBy::Value) {
		}

		if (v.arraySize > 0) {
			mproto << " (&" << v.name << ")[" << v.arraySize << "]";
		} else {
			mproto << " " << v.name;
		}

		if (v.isSampler() && layout.binding != -1) {
			mproto << " = video::TextureUnit::" << convertToTexUnit(layout.binding);
		}

		if (v.arraySize == -1) {
			mproto << ", int amount";
		}
		mproto << ") const";
		methods << "\nbool " << filename << "::" << mproto.str() << " {\n";
		prototypes << "\n";
		prototypes << "\t/**\n";
		prototypes << "\t * @brief Set the shader uniform value for " << v.name << "\n";
		prototypes << "\t * @note The uniform setter uses an internal cache and only perform the real update if something has changed.\n";
		prototypes << "\t */\n";
		prototypes << "\tbool " << mproto.str() << ";\n";
		methods << "\tconst int location = ";
		if (layout.location != -1) {
			methods << layout.location << ";\n";
		} else {
			methods << "getUniformLocation(\"" << v.name << "\");\n";
			methods << "\tif (location == -1) {\n";
			methods << "\t\treturn false;\n";
			methods << "\t}\n";
		}
		methods << "\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize == -1 ? 2 : v.arraySize);
		methods << "(location, " << v.name;
		if (v.arraySize > 0) {
			methods << ", " << v.arraySize;
		} else if (v.arraySize == -1) {
			methods << ", amount";
		}
		methods << ");\n";
		methods << "\treturn true;\n";
		methods << "}\n";

		if (v.isSampler()) {
			if (layout.binding != -1) {
				prototypes << "\n\tvideo::TextureUnit getBound" << uniformName << "TexUnit() const;\n";
				methods << "\n\nvideo::TextureUnit " << filename << "::getBound" << uniformName << "TexUnit() const {\n";
				methods << "\treturn video::TextureUnit::" << convertToTexUnit(layout.binding) << ";\n}\n";
			}
		}
		if (v.isSampler() || v.isImage()) {
			if (layout.imageFormat != video::ImageFormat::Max) {
				prototypes << "\n\tvideo::ImageFormat getImageFormat" << uniformName << "() const;\n";
				methods << "\nvideo::ImageFormat " << filename << "::getImageFormat" << uniformName << "() const {\n";
				methods << "\treturn video::ImageFormat::" << util::getImageFormatTypeString(layout.imageFormat) << ";\n}\n";
			}
		}
		if (layout.localSize.x != -1) {
			prototypes << "\n\tint getLocalSizeX() const;\n";
			methods << "\nint getLocalSizeX() const {\n";
			methods << "\treturn " << layout.localSize.x << ";\n\t}\n";
		}
		if (layout.localSize.y != -1) {
			prototypes << "\n\tint getLocalSizeY() const;\n";
			methods << "\nint getLocalSizeY() const {\n";
			methods << "\treturn " << layout.localSize.y << ";\n\t}\n";
		}
		if (layout.localSize.z != -1) {
			prototypes << "\n\tint getLocalSizeY() const;\n";
			methods << "\nint getLocalSizeZ() const {\n";
			methods << "\treturn " << layout.localSize.z << ";\n\t}\n";
		}

		if (v.arraySize > 0) {
			prototypes << "\n\tbool set" << uniformName << "(" << "const std::vector<" << cType.ctype << ">& var) const;\n\n";
			methods << "\nbool " << filename << "::set" << uniformName << "(" << "const std::vector<" << cType.ctype << ">& var) const {\n";
			methods << "\tconst int location = getUniformLocation(\"" << v.name;
			methods << "\");\n\tif (location == -1) {\n";
			methods << "\t\treturn false;\n";
			methods << "\t}\n";
			methods << "\tcore_assert((int)var.size() == " << v.arraySize << ");\n";
			methods << "\tsetUniform" << util::uniformSetterPostfix(v.type, v.arraySize) << "(location, &var.front(), var.size());\n";
			methods << "\treturn true;\n";
			methods << "}\n";
		} else if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4) {
			prototypes << "\n\tbool set" << uniformName << "(" << "const std::vector<float>& var) const;\n";
			methods << "\nbool " << filename << "::set" << uniformName << "(" << "const std::vector<float>& var) const {\n";
			methods << "\tconst int location = getUniformLocation(\"" << v.name;
			methods << "\");\n\tif (location == -1) {\n";
			methods << "\t\treturn false;\n";
			methods << "\t}\n";
			methods << "\tcore_assert(int(var.size()) % " << cType.components << " == 0);\n";
			methods << "\tsetUniformfv(location, &var.front(), " << cType.components << ", " << cType.components << ");\n";
			methods << "\treturn true;\n";
			methods << "}\n";
		}
		if (i < uniformSize- - 2) {
			methods << "\n";
		}
	}
	for (int i = 0; i < attributeSize; ++i) {
		const Variable& v = shaderStruct.attributes[i];
		const std::string& attributeName = util::convertName(v.name, true);

		prototypes << "\n\t/**\n";
		prototypes << "\t * @brief This version takes the c++ data type as a reference\n";
		prototypes << "\t */\n";
		prototypes << "\ttemplate<typename CLASS, typename TYPE>\n";
		prototypes << "\tvideo::Attribute get" << attributeName << "Attribute(int32_t bufferIndex, TYPE CLASS::* member, bool normalized = false) const {\n";
		prototypes << "\t\tvideo::Attribute attribute" << attributeName << ";\n";
		prototypes << "\t\tattribute" << attributeName << ".bufferIndex = bufferIndex;\n";
		prototypes << "\t\tattribute" << attributeName << ".location = getLocation" << attributeName << "();\n";
		prototypes << "\t\tattribute" << attributeName << ".size = getComponents" << attributeName << "();\n";
		prototypes << "\t\tattribute" << attributeName << ".offset = reinterpret_cast<std::size_t>(&(((CLASS*)nullptr)->*member));\n";
		prototypes << "\t\tattribute" << attributeName << ".stride = sizeof(CLASS);\n";
		prototypes << "\t\tattribute" << attributeName << ".normalized = normalized;\n";
		prototypes << "\t\tattribute" << attributeName << ".type = video::mapType<TYPE>();\n";
		// TODO: add validation that the given c++ data type fits the specified glsl type.
		prototypes << "\t\treturn attribute" << attributeName << ";\n";
		prototypes << "\t}\n";

		prototypes << "\n\t/**\n\t * @brief Return the binding location of the shader attribute @c " << attributeName << "\n\t */\n";
		prototypes << "\tinline int getLocation" << attributeName << "() const {\n";
		prototypes << "\t\treturn getAttributeLocation(\"" << v.name << "\");\n";
		prototypes << "\t}\n";

		prototypes << "\n\t/**\n\t * @brief Return the components if the attribute @c " << attributeName << " is a vector type, or 1 if it is no vector\n\t */\n";
		prototypes << "\tstatic inline int getComponents" << attributeName << "() {\n";
		prototypes << "\t\treturn " << util::getComponents(v.type) << ";\n";
		prototypes << "\t}\n";

		prototypes << "\n\t/**\n\t * @brief Used for instance rendering\n\t */\n";
		prototypes << "\tbool set" << attributeName << "Divisor(uint32_t divisor) const;\n\n";
		methods << "\nbool " << filename << "::set" << attributeName << "Divisor(uint32_t divisor) const {\n";
		methods << "\tconst int location = getAttributeLocation(\"" << v.name << "\");\n";
		methods << "\treturn setDivisor(location, divisor);\n";
		methods << "}\n";

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
	const size_t uniformBlockAmount = shaderStruct.uniformBlocks.size();
	const std::string uniformBufferClassName = util::convertName(shaderStruct.name + "Data", true);
	for (auto & ubuf : shaderStruct.uniformBlocks) {
		const std::string& uniformBufferStructName = util::convertName(ubuf.name, true);
		const std::string& uniformBufferName = util::convertName(ubuf.name, false);
		ub << "\n\t/**\n\t * @brief Uniform buffer for " << uniformBufferStructName << "Data\n\t */\n";
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
		ub << "\t#pragma pack(push, 1)\n\tstruct " << uniformBufferStructName << "Data {\n";
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
		ub << "\tstatic_assert(sizeof(" << uniformBufferStructName << "Data) == " << structSize << ", \"Unexpected structure size for " << uniformBufferStructName << "Data\");\n";
#endif
		ub << "\n\tinline bool update(const " << uniformBufferStructName << "Data& var) {\n";
		ub << "\t\treturn _" << uniformBufferName << ".update((const void*)&var, sizeof(var));\n";
		ub << "\t}\n\n";
		ub << "\n\tinline bool create(const " << uniformBufferStructName << "Data& var) {\n";
		ub << "\t\treturn _" << uniformBufferName << ".create((const void*)&var, sizeof(var));\n";
		ub << "\t}\n\n";
		if (uniformBlockAmount == 1) {
			ub << "\n\tinline operator const video::UniformBuffer&() const {\n";
			ub << "\t\treturn _" << uniformBufferName << ";\n";
			ub << "\t}\n\n";
		}
		ub << "\n\tinline const video::UniformBuffer& get" << uniformBufferStructName << "UniformBuffer() const {\n";
		ub << "\t\treturn _" << uniformBufferName << ";\n";
		ub << "\t}\n";
		prototypes << "\n\t/**\n";
		prototypes << "\t * @brief The the uniform buffer for the uniform block " << ubuf.name << "\n";
		prototypes << "\t */\n";
		prototypes << "\tinline bool set" << uniformBufferStructName << "(const video::UniformBuffer& buf) {\n";
		prototypes << "\t\treturn setUniformBuffer(\"" << ubuf.name << "\", buf);\n";
		prototypes << "\t}\n";

		std::string generatedUb = core::string::replaceAll(templateUniformBuffer, "$name$", uniformBufferClassName);
		generatedUb = core::string::replaceAll(generatedUb, "$namespace$", namespaceSrc);
		generatedUb = core::string::replaceAll(generatedUb, "$uniformbuffers$", ub.str());
		generatedUb = core::string::replaceAll(generatedUb, "$methods$", "");
		generatedUb = core::string::replaceAll(generatedUb, "$shutdown$", shutdown.str());

		const std::string targetFileUb = sourceDirectory + uniformBufferClassName + ".h";

		includes << "#include \"" << uniformBufferClassName + ".h\"\n";

		Log::debug("Generate ubo bindings for %s at %s", uniformBufferStructName.c_str(), targetFileUb.c_str());
		if (!filesystem->syswrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			return false;
		}
	}

	for (const auto& e : shaderStruct.constants) {
		prototypes << "\t/**\n";
		prototypes << "\t * @brief Exported from shader code by @code $constant " << e.first << " " << e.second << " @endcode\n";
		prototypes << "\t */\n";
		if (core::string::isInteger(e.second)) {
			prototypes << "\tinline static constexpr int get" << util::convertName(e.first, true) << "() {\n";
			prototypes << "\t\treturn " << e.second << ";\n";
			prototypes << "\t}\n";
		} else if (core::string::isNumber(e.second)) {
			prototypes << "\tinline static constexpr double get" << util::convertName(e.first, true) << "() {\n";
			prototypes << "\t\treturn " << e.second << ";\n";
			prototypes << "\t}\n";
		} else {
			prototypes << "\tinline static constexpr const char* get" << util::convertName(e.first, true) << "() {\n";
			prototypes << "\t\treturn \"" << e.second << "\";\n";
			prototypes << "\t}\n";
		}
	}

	srcHeader = core::string::replaceAll(srcHeader, "$name$", filename);
	srcHeader = core::string::replaceAll(srcHeader, "$namespace$", namespaceSrc);
	srcHeader = core::string::replaceAll(srcHeader, "$filename$", shaderDirectory + shaderStruct.filename);
	srcHeader = core::string::replaceAll(srcHeader, "$uniformarrayinfo$", uniformArrayInfo.str());
	srcHeader = core::string::replaceAll(srcHeader, "$uniforms$", uniforms.str());

	srcHeader = core::string::replaceAll(srcHeader, "$attributes$", attributes.str());
	srcHeader = core::string::replaceAll(srcHeader, "$methods$", methods.str());
	srcHeader = core::string::replaceAll(srcHeader, "$prototypes$", prototypes.str());
	srcHeader = core::string::replaceAll(srcHeader, "$includes$", includes.str());

	srcHeader = core::string::replaceAll(srcHeader, "$vertexshaderbuffer$", vertexBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$computeshaderbuffer$", computeBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$fragmentshaderbuffer$", fragmentBuffer);
	srcHeader = core::string::replaceAll(srcHeader, "$geometryshaderbuffer$", geometryBuffer);

	srcSource = core::string::replaceAll(srcSource, "$name$", filename);
	srcSource = core::string::replaceAll(srcSource, "$namespace$", namespaceSrc);
	srcSource = core::string::replaceAll(srcSource, "$filename$", shaderDirectory + shaderStruct.filename);
	srcSource = core::string::replaceAll(srcSource, "$uniformarrayinfo$", uniformArrayInfo.str());
	srcSource = core::string::replaceAll(srcSource, "$uniforms$", uniforms.str());

	srcSource = core::string::replaceAll(srcSource, "$attributes$", attributes.str());
	srcSource = core::string::replaceAll(srcSource, "$methods$", methods.str());
	srcSource = core::string::replaceAll(srcSource, "$prototypes$", prototypes.str());
	srcSource = core::string::replaceAll(srcSource, "$includes$", includes.str());

	srcSource = core::string::replaceAll(srcSource, "$vertexshaderbuffer$", maxStringLength(vertexBuffer));
	srcSource = core::string::replaceAll(srcSource, "$computeshaderbuffer$", maxStringLength(computeBuffer));
	srcSource = core::string::replaceAll(srcSource, "$fragmentshaderbuffer$", maxStringLength(fragmentBuffer));
	srcSource = core::string::replaceAll(srcSource, "$geometryshaderbuffer$", maxStringLength(geometryBuffer));

	Log::debug("Generate shader bindings for %s", shaderStruct.name.c_str());
	const std::string targetHeaderFile = sourceDirectory + filename + ".h" + postfix;
	if (!filesystem->syswrite(targetHeaderFile, srcHeader)) {
		Log::error("Failed to write %s", targetHeaderFile.c_str());
		return false;
	}

	const std::string targetSourceFile = sourceDirectory + filename + ".cpp" + postfix;
	if (!filesystem->syswrite(targetSourceFile, srcSource)) {
		Log::error("Failed to write %s", targetSourceFile.c_str());
		return false;
	}
	return true;
}

}
