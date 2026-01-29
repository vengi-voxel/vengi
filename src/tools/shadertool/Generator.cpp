/**
 * @file
 */

#include "Generator.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
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
	if (input.size() > 16380) {
		Log::debug("Need to split the shader source string");
		return "R\"(" + core::string::replaceAll(input, "\n", "\n)\"\nR\"(") + ")\"";
	}
	return "R\"(" + input + ")\"";
}

bool generateSrc(const core::String& templateHeader, const core::String& templateSource, const core::String& templateConstantsHeader,
		const core::String& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const core::String& namespaceSrc, const core::String& sourceDirectory, const core::String& shaderDirectory, const core::String& postfix,
		const core::String& vertexBuffer, const core::String& geometryBuffer, const core::String& fragmentBuffer, const core::String& computeBuffer) {
	core::String srcHeader(templateHeader);
	core::String srcSource(templateSource);
	core::String srcConstantsHeader(templateConstantsHeader);

	const core::String& name = shaderStruct.name + "Shader";
	const core::String& filename = util::convertName(name, true);
	core::String uniforms;
	core::String uniformArrayInfo;
	const int uniformSize = int(shaderStruct.uniforms.size());
	if (uniformSize > 0) {
		uniforms += "checkUniforms({";
		int i = 0;
		for (const Variable& uniform : shaderStruct.uniforms) {
			uniforms += "\"";
			uniforms += uniform.name;
			uniforms += "\"";
			if (i < uniformSize - 1) {
				uniforms += ", ";
			}
			++i;
		}
		uniforms += "});";

		for (const Variable& uniform : shaderStruct.uniforms) {
			uniformArrayInfo += "\tsetUniformArraySize(\"";
			uniformArrayInfo += uniform.name;
			uniformArrayInfo += "\", ";
			uniformArrayInfo += core::string::toString(uniform.arraySize);
			uniformArrayInfo += ");\n";
		}
	} else {
		uniforms += "// no uniforms";
	}

	core::String attributes;
	const int attributeSize = int(shaderStruct.attributes.size());
	if (attributeSize > 0) {
		attributes += "checkAttributes({";
		int i = 0;
		for (const Variable& v : shaderStruct.attributes) {
			attributes += "\"";
			attributes += v.name;
			attributes += "\"";
			if (i < attributeSize - 1) {
				attributes += ", ";
			}
			++i;
		}
		attributes += "});\n";
	} else {
		attributes += "// no attributes";
	}

	core::String methods;
	core::String prototypes;

	prototypes += "\n\tint getFragmentShaderOutputs() const;\n";
	methods += "int ";
	methods += filename;
	methods += "::getFragmentShaderOutputs() const {\n";
	methods += "\treturn ";
	methods += core::string::toString((int)shaderStruct.outs.size());
	methods += ";\n";
	methods += "}\n";

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
	int n = 0;
	for (const Variable& v : shaderStruct.uniforms) {
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

		if ((v.arraySize > 0 && isInteger) || cType.passBy == PassBy::Reference) {
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
		methods += "\nbool ";
		methods += filename;
		methods += "::";
		methods += mproto;

		if (v.isSampler() && layout.binding != -1) {
			mproto += " = video::TextureUnit::";
			mproto += convertToTexUnit(layout.binding);
		}

		if (v.arraySize == -1) {
			mproto += ", int amount";
			methods += ", int amount";
		}
		mproto += ") const";
		methods += ") const";
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
			prototypes += "const core::Array<";
			prototypes += cType.ctype;
			prototypes += ", ";
			prototypes += core::string::toString(v.arraySize);
			prototypes += ">& var) const;\n\n";

			methods += "\nbool ";
			methods += filename;
			methods += "::set";
			methods += uniformName;
			methods += "(";
			methods += "const core::Array<";
			methods += cType.ctype;
			methods += ", ";
			methods += core::string::toString(v.arraySize);
			methods += ">& var) const {\n";
			methods += "\tconst int location = getUniformLocation(\"";
			methods += v.name;
			methods += "\");\n\tif (location == -1) {\n";
			methods += "\t\treturn false;\n";
			methods += "\t}\n";
			methods += "\tsetUniform";
			methods += util::uniformSetterPostfix(v.type, v.arraySize);
			methods += "(location, &var[0], var.size());\n";
			methods += "\treturn true;\n";
			methods += "}\n";
		}
		if (n < uniformSize - 2) {
			methods += "\n";
		}
		++n;
	}
	int i = 0;
	for (const Variable& v : shaderStruct.attributes) {
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
		prototypes += ".offset = reinterpret_cast<size_t>(&(((CLASS*)nullptr)->*member));\n";
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

		if (i < attributeSize - 1) {
			methods += "\n";
		}
		++i;
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
		default:
			ub += "error";
			break;
		}
		ub += ") aligned uniform block structure\n\t */\n";
		ub += "\t#pragma pack(push, 1)\n\tstruct ";
		ub += uniformBufferStructName;
		ub += "Data {\n";

		int offsetsIndex = 0;
		core::String offsets;
		offsets += "\n\tstatic constexpr const uint32_t ";
		offsets += ubuf.name;
		offsets += "_offsets[] = {";

		size_t offset = 0u;
		int paddingCnt = 0;
		for (auto& v : ubuf.members) {
			const int align = ubuf.layout.typeAlign(v);
			int padding = 0;
			if (align <= 0) {
				Log::error("Failed to determine alignment of uniform %s", v.name.c_str());
				return false;
			}
			while (offset % align != 0) {
				++offset;
				++padding;
			}
			if (padding) {
				if (padding > 1) {
					ub += core::String::format("\t\tuint32_t _padding%i[%i];\n", paddingCnt, padding);
				} else {
					ub += core::String::format("\t\tuint32_t _padding%i;\n", paddingCnt);
				}
				++paddingCnt;
			}
			const core::String& uniformName = util::convertName(v.name, false);
			const Types& cType = util::resolveTypes(v.type);
			ub += "\t\t";
			ub += cType.ctype;
			ub += " ";
			ub += uniformName;
			const size_t intSize = ubuf.layout.typeSize(v);
			if (intSize == 0) {
				Log::error("Failed to determine size of uniform %s", v.name.c_str());
				return false;
			}
			if (v.arraySize > 0) {
				ub += "[";
				ub += core::string::toString(v.arraySize);
				ub += "]";
			}
			ub += "; // ";
			ub += core::string::toString((uint32_t)(intSize * 4));
			ub += " bytes - offset ";
			ub += core::string::toString((uint32_t)(offset * 4));
			ub += " - alignment ";
			ub += core::string::toString(align);
			ub += "\n";

			uniforms += "\n\tif (";
			uniforms += core::string::toString((uint32_t)(offset * 4));
			uniforms += " != getUniformBufferOffset(\"";
			uniforms += v.name;
			if (v.arraySize > 0) {
				uniforms += "[0]";
			}
			uniforms += "\")) {\n";
			uniforms += "\t\tLog::error(\"Invalid offset found for uniform ";
			uniforms += v.name;
			if (v.arraySize > 0) {
				uniforms += "[0]";
			}
			uniforms += " %i - expected ";
			uniforms += core::string::toString((uint32_t)(offset * 4));
			uniforms += "\", getUniformBufferOffset(\"";
			uniforms += v.name;
			uniforms += "\"));\n";
			//uniforms += "\t\treturn false;\n";
			uniforms += "\t}\n";

			if (offsetsIndex > 0) {
				offsets += ", ";
			}
			offsets += core::string::toString((uint32_t)(offset * 4));
			++offsetsIndex;

			offset += intSize;
		}
		const size_t fillBytes = (offset * 4) % 16;
		if (fillBytes > 0) {
			// the minimum alignment is 16 bytes
			ub += core::String::format("\t\tuint32_t _padding%i[%i];\n", paddingCnt, (16 - (int)fillBytes) / 4);
			offset += fillBytes / 4;
		}
		ub += "\t};\n\t#pragma pack(pop)\n";
		ub += "\tstatic_assert(sizeof(";
		ub += uniformBufferStructName;
		ub += "Data) == ";
		ub += core::string::toString((uint32_t)(offset * 4));
		ub += ", \"Unexpected structure size for ";
		ub += uniformBufferStructName;
		ub += "Data\");\n";

		offsets += "};\n";
		ub += offsets;

		ub += "\n\tstatic constexpr const char *";
		ub += ubuf.name;
		ub += "_names[] = {";
		int nameIndex = 0;
		for (auto& v : ubuf.members) {
			if (nameIndex > 0) {
				ub += ", ";
			}
			ub += "\"";
			ub += v.name;
			ub += "\"";
			++nameIndex;
		}
		ub += "};\n";

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
		prototypes += "\t * @brief The uniform buffer for the uniform block ";
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
		if (!io::Filesystem::sysWrite(targetFileUb, generatedUb)) {
			Log::error("Failed to write %s", targetFileUb.c_str());
			return false;
		}
	}

	if (!shaderStruct.bufferBlocks.empty()) {
		methods += "\n";
	}

	// Generate SSBO (Shader Storage Buffer Object) structs
	for (auto & buf : shaderStruct.bufferBlocks) {
		const core::String& bufferStructName = util::convertName(buf.name, true);
		const core::String& bufferName = util::convertName(buf.name, false);
		const core::String bufferClassName = util::convertName(shaderStruct.name + "SSBO", true);

		core::String ssbo;
		core::String ssboShutdown;

		ssbo += "\n\t/**\n\t * @brief Shader Storage Buffer for ";
		ssbo += bufferStructName;
		ssbo += "Data\n\t */\n";
		ssbo += "\tvideo::ShaderStorageBuffer _";
		ssbo += bufferName;
		ssbo += ";\n";
		ssboShutdown += "\t\t_";
		ssboShutdown += bufferName;
		ssboShutdown += ".shutdown();\n";

		ssbo += "\t/**\n\t * @brief layout(";
		switch (buf.layout.blockLayout) {
		case BlockLayout::std430:
			ssbo += "std430";
			break;
		case BlockLayout::std140:
			ssbo += "std140";
			break;
		default:
			ssbo += "unknown";
			break;
		}
		ssbo += ") aligned shader storage buffer structure\n\t */\n";
		ssbo += "\t#pragma pack(push, 1)\n\tstruct ";
		ssbo += bufferStructName;
		ssbo += "Data {\n";

		size_t offset = 0u;
		int paddingCnt = 0;
		bool hasDynamicArray = false;
		for (auto& v : buf.members) {
			const int align = buf.layout.typeAlign(v);
			int padding = 0;
			if (align <= 0) {
				Log::error("Failed to determine alignment of ssbo member %s", v.name.c_str());
				return false;
			}
			while (offset % align != 0) {
				++offset;
				++padding;
			}
			if (padding) {
				if (padding > 1) {
					ssbo += core::String::format("\t\tuint32_t _padding%i[%i];\n", paddingCnt, padding);
				} else {
					ssbo += core::String::format("\t\tuint32_t _padding%i;\n", paddingCnt);
				}
				++paddingCnt;
			}
			const core::String& memberName = util::convertName(v.name, false);
			const Types& cType = util::resolveTypes(v.type);
			ssbo += "\t\t";
			ssbo += cType.ctype;
			ssbo += " ";
			ssbo += memberName;
			const size_t intSize = buf.layout.typeSize(v);
			if (intSize == 0 && v.arraySize != -1) {
				Log::error("Failed to determine size of ssbo member %s", v.name.c_str());
				return false;
			}
			if (v.arraySize > 0) {
				ssbo += "[";
				ssbo += core::string::toString(v.arraySize);
				ssbo += "]";
			} else if (v.arraySize == -1) {
				// Dynamic array - must be last member
				hasDynamicArray = true;
				ssbo += "[1]"; // Placeholder, actual size determined at runtime
			}
			ssbo += "; // ";
			if (v.arraySize == -1) {
				ssbo += "dynamic array";
			} else {
				ssbo += core::string::toString((uint32_t)(intSize * 4));
				ssbo += " bytes - offset ";
				ssbo += core::string::toString((uint32_t)(offset * 4));
			}
			ssbo += " - alignment ";
			ssbo += core::string::toString(align);
			ssbo += "\n";

			if (v.arraySize != -1) {
				offset += intSize;
			}
		}

		ssbo += "\t};\n\t#pragma pack(pop)\n";

		if (!hasDynamicArray) {
			// Only add size assertion for fixed-size structs
			const size_t fillBytes = (offset * 4) % 16;
			if (fillBytes > 0) {
				ssbo += core::String::format("\t// Note: struct size is %u bytes, may need padding to 16-byte boundary for some uses\n", (uint32_t)(offset * 4));
			}
			ssbo += "\tstatic_assert(sizeof(";
			ssbo += bufferStructName;
			ssbo += "Data) == ";
			ssbo += core::string::toString((uint32_t)(offset * 4));
			ssbo += ", \"Unexpected structure size for ";
			ssbo += bufferStructName;
			ssbo += "Data\");\n";
		}

		ssbo += "\n\t/**\n\t * @brief Binding index for the shader storage buffer\n\t */\n";
		ssbo += "\tstatic constexpr int ";
		ssbo += bufferName;
		ssbo += "_binding = ";
		ssbo += core::string::toString(buf.layout.binding);
		ssbo += ";\n";

		ssbo += "\n\t/**\n\t * @brief Create the shader storage buffer with the given data\n\t */\n";
		ssbo += "\tinline bool create(const ";
		ssbo += bufferStructName;
		ssbo += "Data* data, size_t count = 1) {\n";
		ssbo += "\t\treturn _";
		ssbo += bufferName;
		ssbo += ".create(data, sizeof(";
		ssbo += bufferStructName;
		ssbo += "Data) * count);\n";
		ssbo += "\t}\n";

		ssbo += "\n\t/**\n\t * @brief Update the shader storage buffer with the given data\n\t */\n";
		ssbo += "\tinline bool update(const ";
		ssbo += bufferStructName;
		ssbo += "Data* data, size_t count = 1) {\n";
		ssbo += "\t\treturn _";
		ssbo += bufferName;
		ssbo += ".update(data, sizeof(";
		ssbo += bufferStructName;
		ssbo += "Data) * count);\n";
		ssbo += "\t}\n";

		ssbo += "\n\t/**\n\t * @brief Bind the buffer to its binding point\n\t */\n";
		ssbo += "\tinline bool bind() const {\n";
		ssbo += "\t\treturn _";
		ssbo += bufferName;
		ssbo += ".bind(";
		ssbo += bufferName;
		ssbo += "_binding);\n";
		ssbo += "\t}\n";

		ssbo += "\n\t/**\n\t * @brief Get the underlying buffer\n\t */\n";
		ssbo += "\tinline video::ShaderStorageBuffer& get";
		ssbo += bufferStructName;
		ssbo += "Buffer() {\n";
		ssbo += "\t\treturn _";
		ssbo += bufferName;
		ssbo += ";\n";
		ssbo += "\t}\n";

		ssbo += "\n\t/**\n\t * @brief Get the underlying buffer (const)\n\t */\n";
		ssbo += "\tinline const video::ShaderStorageBuffer& get";
		ssbo += bufferStructName;
		ssbo += "Buffer() const {\n";
		ssbo += "\t\treturn _";
		ssbo += bufferName;
		ssbo += ";\n";
		ssbo += "\t}\n";

		// Generate a separate header file for the SSBO if template is available
		core::String generatedSsbo = core::string::replaceAll(templateUniformBuffer, "$name$", bufferClassName);
		generatedSsbo = core::string::replaceAll(generatedSsbo, "$namespace$", namespaceSrc);
		generatedSsbo = core::string::replaceAll(generatedSsbo, "$uniformbuffers$", ssbo);
		generatedSsbo = core::string::replaceAll(generatedSsbo, "$methods$", "");
		generatedSsbo = core::string::replaceAll(generatedSsbo, "$shutdown$", ssboShutdown);

		// Replace UniformBuffer include with ShaderStorageBuffer include for SSBOs
		generatedSsbo = core::string::replaceAll(generatedSsbo, "#include \"video/UniformBuffer.h\"", "#include \"video/ShaderStorageBuffer.h\"");

		const core::String targetFileSsbo = sourceDirectory + bufferClassName + ".h";

		includes += "#include \"";
		includes += bufferClassName;
		includes += ".h\"\n";

		Log::debug("Generate ssbo bindings for %s at %s", bufferStructName.c_str(), targetFileSsbo.c_str());
		if (!io::Filesystem::sysWrite(targetFileSsbo, generatedSsbo)) {
			Log::error("Failed to write %s", targetFileSsbo.c_str());
			return false;
		}

		// Also add binding getter to the shader class
		prototypes += "\n\t/**\n";
		prototypes += "\t * @brief Get the binding index of the shader storage buffer ";
		prototypes += buf.name;
		prototypes += "\n";
		prototypes += "\t */\n";
		prototypes += "\tinline int getBinding";
		prototypes += bufferStructName;
		prototypes += "() {\n";
		prototypes += "\t\treturn ";
		prototypes += core::string::toString(buf.layout.binding);
		prototypes += ";\n";
		prototypes += "\t}\n";
	}

	core::String constants;
	constants.reserve(4096);
	for (const auto& e : shaderStruct.constants) {
		constants += "\t/**\n";
		constants += "\t * @brief Exported from shader code by @code $constant ";
		constants += e->key;
		constants += " ";
		constants += e->value;
		constants += " @endcode\n";
		constants += "\t */\n";
		if (core::string::isIntegerWithPostfix(e->value)) {
			constants += "\tinline static constexpr int get";
			constants += util::convertName(e->key, true);
			constants += "() {\n";
			constants += "\t\treturn ";
			constants += e->value;
			constants += ";\n";
			constants += "\t}\n";
		} else if (core::string::isNumber(e->value)) {
			constants += "\tinline static constexpr double get";
			constants += util::convertName(e->key, true);
			constants += "() {\n";
			constants += "\t\treturn ";
			constants += e->value;
			constants += ";\n";
			constants += "\t}\n";
		} else {
			constants += "\tinline static constexpr const char* get";
			constants += util::convertName(e->key, true);
			constants += "() {\n";
			constants += "\t\treturn \"";
			constants += e->value;
			constants += "\";\n";
			constants += "\t}\n";
		}
	}

	if (shaderStruct.constants.empty()) {
		constants += "#error \"Shader does not define any constants\"\n";
	}

	srcConstantsHeader = core::string::replaceAll(srcConstantsHeader, "$name$", filename);
	srcConstantsHeader = core::string::replaceAll(srcConstantsHeader, "$namespace$", namespaceSrc);
	srcConstantsHeader = core::string::replaceAll(srcConstantsHeader, "$prototypes$", constants);

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
	if (!io::Filesystem::sysWrite(targetHeaderFile, srcHeader)) {
		Log::error("Failed to write %s", targetHeaderFile.c_str());
		return false;
	}

	const core::String targetSourceFile = sourceDirectory + filename + ".cpp" + postfix;
	if (!io::Filesystem::sysWrite(targetSourceFile, srcSource)) {
		Log::error("Failed to write %s", targetSourceFile.c_str());
		return false;
	}

	const core::String targetConstantHeaderFile = sourceDirectory + filename + "Constants.h" + postfix;
	if (!io::Filesystem::sysWrite(targetConstantHeaderFile, srcConstantsHeader)) {
		Log::error("Failed to write %s", targetConstantHeaderFile.c_str());
		return false;
	}

	return true;
}

}
